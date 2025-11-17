#include "oculusImpl.h"

#ifdef AN_WITH_OCULUS
#include "..\vrFovPort.h"

#include "..\..\mainConfig.h"

#include "..\..\math\math.h"
#include "..\..\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\mesh\mesh3d.h"
#include "..\..\splash\splashLogos.h"
#include "..\..\system\core.h"
#include "..\..\system\sound\soundSystem.h"
#include "..\..\system\video\font.h"
#include "..\..\system\video\video3d.h"
#include "..\..\system\video\indexFormat.h"
#include "..\..\system\video\renderTarget.h"
#include "..\..\system\video\video3dPrimitives.h"

#include "..\..\containers\arrayStack.h"

#include "..\..\performance\performanceUtils.h"

#include "OVR_CAPI_Audio.h"
#include "OVR_CAPI_GL.h"
#include "OVR_CAPI_GL.h"

using namespace VR;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_DEVELOPMENT
#define DEBUG_DECIDE_HANDS
#endif

//

// check if we've got the right lib version

#if OVR_PRODUCT_VERSION != 1
#error invalid ovr product version, get the right lib (code_externals) or update version here
#endif
#if OVR_MAJOR_VERSION != 1
#error invalid ovr major version, get the right lib (code_externals) or update version here
#endif
#if OVR_MINOR_VERSION != 43
#error invalid ovr minor version, get the right lib (code_externals) or update version here
#endif

//

static tchar* vrModelPrefix = TXT("oculus__");

#define TRACKPAD_DIR_THRESHOLD 0.3f
#define JOYSTICK_DIR_THRESHOLD 0.5f

static VectorInt2 get_vector_int_2_from(ovrSizei const & _size)
{
	return VectorInt2(_size.w, _size.h);
}

static Vector3 get_vector_from(ovrVector3f const & _v)
{
	// oculus: x right y up -z forward
	// our: x right y forward z up
	return Vector3(_v.x, -_v.z, _v.y);
}

static Vector2 get_vector_2_from(ovrVector2f const & _v)
{
	return Vector2(_v.x, _v.y);
}

static Quat get_quat_from(ovrQuatf const & _quat)
{
	return Quat(_quat.x, -_quat.z, _quat.y, _quat.w);
}

static Transform get_transform_from(ovrPosef const & _pose)
{
	return Transform(get_vector_from(_pose.Position), get_quat_from(_pose.Orientation));
}

static bool run_ovr(tchar const * _when, ovrResult _result)
{
	if (OVR_SUCCESS(_result))
	{
		return true;
	}
	else
	{
		error_dev_ignore(TXT("oculus error (%i) on: %S"), _result, _when);
		return false;
	}
}

//

bool OculusImpl::is_available()
{
	ovrDetectResult result = ovr_Detect(0);
	return result.IsOculusServiceRunning && result.IsOculusHMDConnected;
}

OculusImpl::OculusImpl(Oculus* _owner)
: base(_owner)
{
}

void OculusImpl::oculus_log_callback(uintptr_t userData, int level, const char* message)
{
	// breaks due to being called from other thread? why?
	String converted = String::convert_from(message);
	if (level & ovrLogLevel_Error)
	{
		error(converted.to_char());
	}
	else
	{
		output(converted.to_char());
	}
}

void OculusImpl::init_impl()
{
	System::Core::access_system_tags().set_tag(Name(TXT("oculus")));
	Splash::Logos::add(TXT("oculus"), SPLASH_LOGO_IMPORTANT);

	output(TXT("initialising oculus"));
	if (!initialisedLibOVR)
	{
		ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, nullptr, 0, 0 };
#ifndef AN_DEVELOPMENT_OR_PROFILER
		initParams.Flags |= ovrInit_FocusAware;
#endif
		libOVR_OK = run_ovr(TXT("ovr_Initialize"), ovr_Initialize(&initParams));

		initialisedLibOVR = true;
	}

	modelNumber = String(TXT("OculusTouch")); // forced
}

void OculusImpl::shutdown_impl()
{
	if (initialisedLibOVR)
	{
		todo_note(TXT("uncomment it when SDK works"));
		//ovr_Shutdown();
		initialisedLibOVR = false;
	}
}

void OculusImpl::create_device_impl()
{
	output(TXT("create device"));
	if (!run_ovr(TXT("ovr_Create"), ovr_Create(&session, &luid)))
	{
		close_device();
		return;
	}

	hmdDesc = ovr_GetHmdDesc(session);
	trackingSystemName = String::from_char8(hmdDesc.ProductName);

	output(TXT("device : %S"), String::from_char8(hmdDesc.ProductName).to_char());

	preferredFullScreenResolution = get_vector_int_2_from(hmdDesc.Resolution);

	// floor at level 0
	run_ovr(TXT("ovr_SetTrackingOriginType"), ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel));

	GUID audioDeviceGUID;
	if (run_ovr(TXT("ovr_GetAudioDeviceOutGuid"), (ovr_GetAudioDeviceOutGuid(&audioDeviceGUID))))
	{
		::System::Sound::set_preferred_audio_device(audioDeviceGUID);
	}

	wireless = String::does_contain_icase(trackingSystemName.to_char(), TXT("quest"));
	todo_hack(TXT("for time being we have no way to read a proper boundary"));
	mayHaveInvalidBoundary = wireless;
}

void OculusImpl::close_device_impl()
{
	close_internal_render_targets();
	if (session)
	{
		ovr_Destroy(session);
	}
	session = nullptr;
}

void OculusImpl::close_internal_render_targets()
{
	for_count(int, i, 2)
	{
		if (renderInfo.eyeTextureSwapChain[i])
		{
			ovr_DestroyTextureSwapChain(session, renderInfo.eyeTextureSwapChain[i]);
			renderInfo.eyeTextureSwapChain[i] = 0;
		}
		if (renderInfo.eyeFrameBufferID[i])
		{
			DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDeleteFramebuffers(1, &renderInfo.eyeFrameBufferID[i]); AN_GL_CHECK_FOR_ERROR
			renderInfo.eyeFrameBufferID[i] = 0;
		}
	}
}

bool OculusImpl::can_load_play_area_rect_impl()
{
	ovrVector3f ovrDimensions;
	if (!run_ovr(TXT("ovr_GetBoundaryDimensions"), ovr_GetBoundaryDimensions(session, ovrBoundary_PlayArea, &ovrDimensions)))
	{
		owner->add_warning(String(TXT("Could not get boundary dimension. Please check device and setup.")));
		output(TXT("couldn't get boundary dimensions"));
		return false;
	}

	Vector3 dimensions = get_vector_from(ovrDimensions);

	dimensions = owner->get_map_vr_space().vector_to_local(dimensions);

	// make values absolute
	dimensions.x = abs(dimensions.x);
	dimensions.y = abs(dimensions.y);

	if (dimensions.x > MAX_VR_SIZE ||
		dimensions.y > MAX_VR_SIZE ||
		dimensions.x < 0.000001f ||
		dimensions.y < 0.000001f)
	{
		owner->add_warning(String(TXT("Could not get proper boundary dimension. Please check device and setup.")));
		output(TXT("couldn't get proper boundary dimensions"));
		return false;
	}

	return true;
}

bool OculusImpl::load_play_area_rect_impl(bool _loadAnything)
{
	ovrVector3f ovrDimensions;

	if (!run_ovr(TXT("ovr_GetBoundaryDimensions"), ovr_GetBoundaryDimensions(session, ovrBoundary_PlayArea, &ovrDimensions)))
	{
		boundaryUnavailable = true;
		output(TXT("couldn't get boundary dimensions"));
		if (_loadAnything)
		{
			owner->set_play_area_rect(Vector3::yAxis * 1.2f * 0.5f, Vector3::xAxis * 1.8f * 0.5f);
			owner->add_warning(String(TXT("Calibrate to get the right play area.")));
			return true;
		}
		else
		{
			owner->add_error(String(TXT("Calibrate to get the right play area.")));
			return false;
		}
	}

	boundaryUnavailable = false; // available

	Transform origin = Transform::identity;

	{
		// read play area first to get the centre, so we will be aligned properly
		// play area is always axis aligned on oculus?
		int count;
		if (run_ovr(TXT("ovr_GetBoundaryGeometry"), ovr_GetBoundaryGeometry(session, ovrBoundary_PlayArea, nullptr, &count)))
		{
			if (count == 0)
			{
				if (!_loadAnything)
				{
					return false;
				}
			}
			else
			{
				allocate_stack_var(ovrVector3f, playAreaPoints, count);
				if (run_ovr(TXT("ovr_GetBoundaryGeometry"), ovr_GetBoundaryGeometry(session, ovrBoundary_PlayArea, playAreaPoints, &count)))
				{
					Vector3 playAreaCentre = Vector3::zero;
					for_count(int, i, count)
					{
						playAreaCentre += get_vector_from(playAreaPoints[i]);
					}
					playAreaCentre /= (float)count;
					playAreaCentre.z = 0.0f;
					origin.set_translation(playAreaCentre);
				}
			}
		}
	}

	owner->set_map_vr_space_origin(origin);

#ifdef AN_INSPECT_VR_PLAY_AREA
	output(TXT("[OCULUS] play area centre read %.3fx%.3f (yaw %.0f)"), origin.get_translation().x, origin.get_translation().y, origin.get_orientation().to_rotator().yaw);
#endif

	// dimensions after origin

	Vector3 dimensions = get_vector_from(ovrDimensions);
	
#ifdef AN_INSPECT_VR_PLAY_AREA
	output(TXT("[OCULUS] play area size read %.3fx%.3f"), dimensions.x, dimensions.y);
#endif

	dimensions = owner->get_map_vr_space_auto().vector_to_local(dimensions);

	// make them absolute
	dimensions.x = abs(dimensions.x);
	dimensions.y = abs(dimensions.y);

	auto rawDimensions = dimensions;

	// make values absolute and clamp
	dimensions.x = clamp(dimensions.x, 0.1f, MAX_VR_SIZE);
	dimensions.y = clamp(dimensions.y, 0.1f, MAX_VR_SIZE);

	owner->set_play_area_rect(dimensions * Vector3::yAxis * 0.5f, dimensions * Vector3::xAxis * 0.5f);

	if ((dimensions - rawDimensions).is_almost_zero())
	{
		// read outer boundary
		Array<Vector2> boundaryPoints;
		int count;
		if (run_ovr(TXT("ovr_GetBoundaryGeometry"), ovr_GetBoundaryGeometry(session, ovrBoundary_Outer, nullptr, &count)))
		{
			allocate_stack_var(ovrVector3f, points, count);
			if (run_ovr(TXT("ovr_GetBoundaryGeometry"), ovr_GetBoundaryGeometry(session, ovrBoundary_Outer, points, &count)))
			{
				for_count(int, i, count)
				{
					boundaryPoints.push_back(get_vector_from(points[i]).to_vector2());
				}
			}
		}

		owner->set_raw_boundary(boundaryPoints);
	}
	else
	{
		owner->set_raw_boundary(Array<Vector2>());
		owner->set_map_vr_space_origin(Transform::identity);
	}

	output(TXT("half right %.3f (full %.3f)"), owner->get_raw_whole_play_area_rect_half_right().length(), dimensions.x);
	output(TXT("half forward %.3f (full %.3f)"), owner->get_raw_whole_play_area_rect_half_forward().length(), dimensions.y);

	owner->act_on_possibly_invalid_boundary();

	if (owner->get_raw_whole_play_area_rect_half_right().length() >= owner->get_raw_whole_play_area_rect_half_forward().length())
	{
		return true;
	}

	return false;
}

void OculusImpl::init_hand_controller_track_device(int _index)
{
	if (handControllerTrackDevices[_index].deviceID == NONE)
	{
		return;
	}

	String modelName;// = get_string_tracked_device_property(openvrSystem, handControllerTrackDevices[_index].controllerType, vr::Prop_RenderModelName_String);
	output(TXT("model : "), modelName.to_char());
	
	if (! modelName.is_empty())
	{
		owner->set_model_name_for_hand(_index, Name(String::printf(TXT("%S%S"), vrModelPrefix, modelName.to_char())));
	}
}

void OculusImpl::close_hand_controller_track_device(int _index)
{
	owner->set_model_name_for_hand(_index, Name::invalid());
}

void OculusImpl::update_track_device_indices()
{
	if (!is_ok())
	{
#ifdef AN_DEVELOPMENT
		// hardcoded for debug purposes
		owner->set_model_name_for_hand(0, Name(TXT("openvr__vr_controller_vive_1_5")));
		owner->set_model_name_for_hand(1, Name(TXT("openvr__vr_controller_vive_1_5")));
#endif
		return;
	}

	for_count(int, i, Hands::Count)
	{
		close_hand_controller_track_device(i);
		handControllerTrackDevices[i].clear();
	}

	devicesMask = ovr_GetConnectedControllerTypes(session);

	int handIndex = 0;
	if (devicesMask & ovrControllerType_LTouch)
	{
		handControllerTrackDevices[handIndex].controllerType = ovrControllerType_LTouch;
		handControllerTrackDevices[handIndex].deviceID = handIndex;
		handControllerTrackDevices[handIndex].hand = Hand::Left;
		init_hand_controller_track_device(handIndex);
		++handIndex;
	}
	if (devicesMask & ovrControllerType_RTouch)
	{
		handControllerTrackDevices[handIndex].controllerType = ovrControllerType_RTouch;
		handControllerTrackDevices[handIndex].deviceID = handIndex;
		handControllerTrackDevices[handIndex].hand = Hand::Right;
		init_hand_controller_track_device(handIndex);
		++handIndex;
	}

	owner->mark_new_controllers();
}

static int controller_type_to_hand(int _type)
{
	if (_type == ovrControllerType_LTouch) return ovrHand_Left;
	if (_type == ovrControllerType_RTouch) return ovrHand_Right;
	an_assert(false);
	return NONE;
}

void OculusImpl::store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, ovrTrackingState& _trackingState)
{
	if (_trackingState.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		_poseSet.rawViewAsRead = get_transform_from(_trackingState.HeadPose.ThePose).to_world(viewInHeadPoseSpace);
	}

	for_count(int, i, Hands::Count)
	{
		int iController = handControllerTrackDevices[i].controllerType;
		Hand::Type handType = handControllerTrackDevices[i].hand;
		_poseSet.hands[i].rawHandTrackingRootPlacementAsRead.clear();
		if (iController != NONE)
		{
			auto & poseAsRead = _poseSet.hands[i].rawPlacementAsRead;
			poseAsRead = get_transform_from(_trackingState.HandPoses[handType].ThePose);
			//handPoseValid[i] = _trackingState.HandStatusFlags[handType] & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked);
			if (_controls)
			{
				auto & controls = _controls->hands[i];
				ovrInputState inputState;
				controls.trackpadTouchDelta = Vector2::zero; // reset delta, we will update it if possible
				if (run_ovr(TXT("ovr_GetInputState"), ovr_GetInputState(session, (ovrControllerType)iController, &inputState)))
				{
					controls.upsideDown.clear();
					controls.insideToHead.clear();
					controls.pointerPinch.clear();
					controls.middlePinch.clear();
					controls.ringPinch.clear();
					controls.pinkyPinch.clear();
					controls.trigger = inputState.IndexTrigger[handType];
					int indexPointing = handType == ovrHand_Right ? ovrTouch_RIndexPointing : ovrTouch_LIndexPointing;
					int thumpUp = handType == ovrHand_Right ? ovrTouch_RThumbUp : ovrTouch_LThumbUp;
					if ((inputState.Touches & indexPointing) && inputState.HandTrigger[handType] > VRControls::AXIS_TO_BUTTON_THRESHOLD)
					{
						controls.pose = 1.0f;
						controls.thumb = (inputState.Touches & thumpUp)? 0.0f : 1.0f;
						controls.pointer = 0.0f;
						controls.middle = 1.0f;
						controls.ring = 1.0f;
						controls.pinky = 1.0f;
						controls.grip = 1.0f;
					}
					else if ((inputState.Touches & thumpUp) && inputState.HandTrigger[handType] > VRControls::AXIS_TO_BUTTON_THRESHOLD)
					{
						controls.pose = 1.0f;
						controls.thumb = 0.0f;
						controls.pointer = 1.0f;
						controls.middle = 1.0f;
						controls.ring = 1.0f;
						controls.pinky = 1.0f;
						controls.grip = 1.0f;
					}
					else
					{
						controls.pose = 0.0f;
						controls.thumb = inputState.HandTrigger[handType];
						controls.pointer = inputState.HandTrigger[handType];
						controls.middle = inputState.HandTrigger[handType];
						controls.ring = inputState.HandTrigger[handType];
						controls.pinky = inputState.HandTrigger[handType];
						controls.grip = inputState.HandTrigger[handType];
					}

					controls.joystick = get_vector_2_from(inputState.Thumbstick[handType]);

					if (handType == ovrHand_Right)
					{
						controls.buttons.primary = (inputState.Buttons & ovrButton_A) != 0;
						controls.buttons.secondary = (inputState.Buttons & ovrButton_B) != 0;
						controls.buttons.joystickPress = (inputState.Buttons & ovrButton_RThumb) != 0;
					}
					else if (handType == ovrHand_Left)
					{
						controls.buttons.primary = (inputState.Buttons & ovrButton_X) != 0;
						controls.buttons.secondary = (inputState.Buttons & ovrButton_Y) != 0;
						controls.buttons.joystickPress = (inputState.Buttons & ovrButton_LThumb) != 0;
					}
					if (owner->get_dominant_hand() != handType)
					{
						controls.buttons.systemMenu = (inputState.Buttons & ovrButton_Enter) != 0;
					}
					else
					{
						controls.buttons.systemMenu = false;
					}

					controls.do_auto_buttons();
				}
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

void OculusImpl::advance_events()
{
	ovrSessionStatus sessionStatus;
	run_ovr(TXT("ovr_GetSessionStatus"), ovr_GetSessionStatus(session, &sessionStatus));
	if (sessionStatus.ShouldQuit)
	{
		System::Core::quick_exit(true);
	}

	if (sessionStatus.ShouldRecenter)
	{
		ovr_ClearShouldRecenterFlag(session);
		owner->access_controls().requestRecenter = true;
		owner->reset_immobile_origin_in_vr_space();
	}

	int newDevicesMask = ovr_GetConnectedControllerTypes(session);
	if (newDevicesMask != devicesMask)
	{
		output(TXT("devices changed!"));
		update_track_device_indices();
		owner->decide_hands();
	} 

	// EVENTS!

	/*
	// do we need it?
	if (sessionStatus.ShouldRecenter)
	{
		ovr_RecenterTrackingOrigin(session);
	}
	*/

	bool newOverlayPresent = sessionStatus.OverlayPresent != 0;
	if (newOverlayPresent ^ overlayPresent)
	{
		if (newOverlayPresent) ::System::Core::pause_vr(bit(1));
						  else ::System::Core::unpause_vr(bit(1));
	}
	overlayPresent = newOverlayPresent;
	bool newHasInputFocus = sessionStatus.HasInputFocus != 0;
#ifndef AN_DEVELOPMENT
	if (newHasInputFocus ^ hasInputFocus)
	{
		if (newHasInputFocus) ::System::Core::unpause_vr(bit(2));
						 else ::System::Core::pause_vr(bit(2));
	}
#endif
	hasInputFocus = newHasInputFocus;
}

void OculusImpl::advance_pulses()
{
	{
		int handIdx = 0;
		while (auto* pulse = owner->get_pulse(handIdx))
		{
			float currentStrength = pulse->get_current_strength() * MainConfig::global().get_vr_haptic_feedback();
			if (currentStrength > 0.0f)
			{
				run_ovr(TXT("ovr_SetControllerVibration"), ovr_SetControllerVibration(session, (ovrControllerType)handControllerTrackDevices[handIdx].controllerType, pulse->get_current_frequency(), pulse->get_current_strength()));
			}
			++handIdx;
		}
	}
}

void OculusImpl::advance_poses()
{
	auto & vrControls = owner->access_controls();
	auto & poseSet = owner->access_controls_pose_set();

	// get poses
	ovrTrackingState trackingState = ovr_GetTrackingState(session, ovr_GetPredictedDisplayTime(session, frameIndex), ovrTrue);
		
	store(poseSet, &vrControls, trackingState);
}

void OculusImpl::next_frame()
{
	if (!is_ok())
	{
		return;
	}
	++frameIndex;
}	

bool OculusImpl::begin_render(System::Video3D * _v3d)
{
	if (!is_ok())
	{
		return false;
	}
	frameIndexToSubmit = frameIndex;
	//ovr_WaitToBeginFrame(session, frameIndexToSubmit);
	//ovr_BeginFrame(session, frameIndexToSubmit);
	// get poses to render - 
	ovrTrackingState trackingState = ovr_GetTrackingState(session, ovr_GetPredictedDisplayTime(session, frameIndexToSubmit), ovrTrue);
	renderPoseSetTimeSeconds = ovr_GetTimeInSeconds();
	ovrPosef HmdToEyePoses[2];
	HmdToEyePoses[0] = renderInfo.eyeRenderDesc[0].HmdToEyePose;
	HmdToEyePoses[1] = renderInfo.eyeRenderDesc[1].HmdToEyePose;
	ovr_CalcEyePoses2(trackingState.HeadPose.ThePose, HmdToEyePoses, eyeRenderPoses);
	store(owner->access_render_pose_set(), nullptr, trackingState);
	owner->store_last_valid_render_pose_set();
	return true;
}

bool OculusImpl::end_render(System::Video3D * _v3d, ::System::RenderTargetPtr* _eyeRenderTargets)
{
	if (!is_ok())
	{
		return false;
	}

	::System::RenderTarget::bind_none();

	for (int eye = 0; eye < 2; ++eye)
	{
		MEASURE_PERFORMANCE(copyTexture);
		/*
		 *	This is a bit nasty.
		 *	We have our own render targets that are output from the game and currently we render them onto textures provided by Oculus
		 *	In the end it would be nice to replace this with set of render targets that were created using textures provided by Oculus
		 */
		todo_note(TXT("replace this copying with proper render targets based on textures provided by Oculus"));
		GLuint curTexId;
		int curIndex;
		run_ovr(TXT("ovr_GetTextureSwapChainCurrentIndex"), ovr_GetTextureSwapChainCurrentIndex(session, renderInfo.eyeTextureSwapChain[eye], &curIndex));
		run_ovr(TXT("ovr_GetTextureSwapChainBufferGL"), ovr_GetTextureSwapChainBufferGL(session, renderInfo.eyeTextureSwapChain[eye], curIndex, &curTexId));
		DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, renderInfo.eyeFrameBufferID[eye]); AN_GL_CHECK_FOR_ERROR
		DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0); AN_GL_CHECK_FOR_ERROR
		GLenum buffers[1];
		GLenum* buffer = buffers;
		int usedCount = 0;
		int index = 0;
		*buffer = GL_COLOR_ATTACHMENT0 + index;
		++buffer;
		++usedCount;
		++index;
		DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDrawBuffers(usedCount, buffers); AN_GL_CHECK_FOR_ERROR

		VectorInt2 renderSize = renderInfo.frameBufferSize[eye];

		_v3d->set_viewport(VectorInt2::zero, renderSize);
		_v3d->set_near_far_plane(0.02f, 100.0f);

		_v3d->set_2d_projection_matrix_ortho();
		_v3d->access_model_view_matrix_stack().clear();

		_eyeRenderTargets[eye]->resolve_forced_unbind();
		_eyeRenderTargets[eye]->render(0, _v3d, Vector2::zero, renderSize.to_vector2());

		DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, renderInfo.eyeFrameBufferID[eye]); AN_GL_CHECK_FOR_ERROR
		DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0); AN_GL_CHECK_FOR_ERROR

		run_ovr(TXT("ovr_CommitTextureSwapChain"), ovr_CommitTextureSwapChain(session, renderInfo.eyeTextureSwapChain[eye]));
	}

	::System::Video3D::get()->invalidate_bound_frame_buffer_info();
	::System::RenderTarget::bind_none();

	{
		MEASURE_PERFORMANCE(flush);
		DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR
	}

	ovrViewScaleDesc viewScaleDesc;
	viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
	viewScaleDesc.HmdToEyePose[0] = renderInfo.eyeRenderDesc[0].HmdToEyePose;
	viewScaleDesc.HmdToEyePose[1] = renderInfo.eyeRenderDesc[0].HmdToEyePose;

	ovrLayerEyeFov ld;
	ld.Header.Type = ovrLayerType_EyeFov;
	ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

	for (int eye = 0; eye < 2; ++eye)
	{
		ld.ColorTexture[eye] = renderInfo.eyeTextureSwapChain[eye];
		ld.Viewport[eye] = renderInfo.eyeViewport[eye];
		ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
		ld.RenderPose[eye] = eyeRenderPoses[eye];
		ld.SensorSampleTime = renderPoseSetTimeSeconds;
	}

	{
		MEASURE_PERFORMANCE(submitFrame);
		ovrLayerHeader* layers = &ld.Header;
		//run_ovr(TXT("ovr_EndFrame"), ovr_EndFrame(session, frameIndexToSubmit, &viewScaleDesc, &layers, 1));
		run_ovr(TXT("ovr_SubmitFrame"), ovr_SubmitFrame(session, frameIndexToSubmit, &viewScaleDesc, &layers, 1));
	}

	// update frame time seconds to get proper expected frame rate!
	frameTimeSeconds[1] = frameTimeSeconds[0];
	frameTimeSeconds[0] = ovr_GetTimeInSeconds();

	return true;
}

void OculusImpl::create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget)
{
	close_internal_render_targets();

	_useDirectToVRRendering = false;
	_noOutputRenderTarget = false;

	for_count(int, eyeIdx, 2)
	{
		// due to glDrawBuffers they have to be the same
		VectorInt2 resolution = renderInfo.eyeResolution[eyeIdx];
		if (resolution.is_zero())
		{
			resolution = _fallbackEyeResolution * renderInfo.renderScale[eyeIdx];
		}
		resolution *= MainConfig::global().get_video().vrResolutionCoef * MainConfig::global().get_video().overall_vrResolutionCoef;
		resolution = apply_aspect_ratio_coef(resolution, MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef);
		renderInfo.frameBufferSize[eyeIdx] = resolution;

		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Width = resolution.x;
		desc.Height = resolution.y;
		desc.MipLevels = 1;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB; // even if we do not use SRGB, using this texture format and not enabling SRGB gives proper result
		desc.SampleCount = 1;
		desc.StaticImage = ovrFalse;
		desc.BindFlags = 0;

		renderInfo.eyeViewport[eyeIdx].Pos.x = 0;
		renderInfo.eyeViewport[eyeIdx].Pos.y = 0;
		renderInfo.eyeViewport[eyeIdx].Size.w = desc.Width;
		renderInfo.eyeViewport[eyeIdx].Size.h = desc.Height;

		ovrResult result = ovr_CreateTextureSwapChainGL(session, &desc, &renderInfo.eyeTextureSwapChain[eyeIdx]);
		an_assert(OVR_SUCCESS(result));

		if (OVR_SUCCESS(result))
		{
			int length = 0;
			ovr_GetTextureSwapChainLength(session, renderInfo.eyeTextureSwapChain[eyeIdx], &length);

			for (int i = 0; i < length; ++i)
			{
				GLuint chainTexId;
				ovr_GetTextureSwapChainBufferGL(session, renderInfo.eyeTextureSwapChain[eyeIdx], i, &chainTexId);
				DIRECT_GL_CALL_ glBindTexture(GL_TEXTURE_2D, chainTexId); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR
			}
		}

		DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glGenFramebuffers(1, &renderInfo.eyeFrameBufferID[eyeIdx]); AN_GL_CHECK_FOR_ERROR
	}
}

void OculusImpl::on_render_targets_created()
{
}

void OculusImpl::setup_rendering_on_init_video(::System::Video3D* _v3d)
{
	an_assert(is_ok(), TXT("openvr should be working"));

	for (int eyeIdx = 0; eyeIdx < 2; ++eyeIdx)
	{
		ovrEyeType ovrEye = (ovrEyeType)eyeIdx;
		Eye::Type eye = (Eye::Type)eyeIdx;
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, (ovrEyeType)eyeIdx, hmdDesc.DefaultEyeFov[eyeIdx], 1);
		ovrFovPort fp;

		fp = hmdDesc.DefaultEyeFov[eyeIdx];
		renderInfo.eyeRenderDesc[eyeIdx] = ovr_GetRenderDesc(session, ovrEye, fp);
		renderInfo.fovPort[eyeIdx] = fp;

		// store render info
		renderInfo.eyeResolution[eye].x = idealTextureSize.w;
		renderInfo.eyeResolution[eye].y = idealTextureSize.h;
		renderInfo.fov[eye] = renderInfo.fovPort[eyeIdx].get_fov();
		renderInfo.aspectRatio[eye] = aspect_ratio(renderInfo.eyeResolution[eye]);
		renderInfo.lensCentreOffset[eye] = renderInfo.fovPort[eyeIdx].get_lens_centre_offset();
		auto hmdToEyeTransformLeft = get_transform_from(renderInfo.eyeRenderDesc[eye].HmdToEyePose);
		renderInfo.eyeOffset[eye] = hmdToEyeTransformLeft;
	}
	viewInHeadPoseSpace = Transform::lerp(0.5f, renderInfo.eyeOffset[0], renderInfo.eyeOffset[1]);
	for (int eyeIdx = 0; eyeIdx < 2; ++eyeIdx)
	{
		renderInfo.eyeOffset[eyeIdx] = viewInHeadPoseSpace.to_local(renderInfo.eyeOffset[eyeIdx]);
	}
	if (renderInfo.eyeOffset[Eye::Left].get_translation().x > 0.0f)
	{
		error_stop(TXT("why eyes are not on the right side?"));
	}
}

Meshes::Mesh3D* OculusImpl::load_mesh(Name const & _modelName) const
{
#ifdef AN_USE_VR_RENDER_MODELS
#endif
	return nullptr;
}

DecideHandsResult::Type OculusImpl::decide_hands(OUT_ int& _leftHand, OUT_ int& _rightHand)
{
	if (!is_ok())
	{
		// we won't ever decide
		return DecideHandsResult::CantTellAtAll;
	}

	DecideHandsResult::Type result = DecideHandsResult::Decided;
#ifdef DEBUG_DECIDE_HANDS
	output(TXT("oculus : decide hands"));
#endif
	_leftHand = NONE;
	_rightHand = NONE;
	for_count(int, i, 2)
	{
		if (handControllerTrackDevices[i].controllerType != NONE)
		{
			if (handControllerTrackDevices[i].controllerType == ovrControllerType_LTouch)
			{
				_leftHand = i;
			}
			else if (handControllerTrackDevices[i].controllerType == ovrControllerType_RTouch)
			{
				_rightHand = i;
			}
			else
			{
				result = DecideHandsResult::UnableToDecide;
			}
		}
		else
		{
#ifdef DEBUG_DECIDE_HANDS
			output(TXT("  hand %i not present"), i);
#endif
		}
	}
	if (result == DecideHandsResult::UnableToDecide &&
		(_leftHand != NONE || _rightHand != NONE))
	{
		result = DecideHandsResult::PartiallyDecided;
	}
	return result;
}

void OculusImpl::force_bounds_rendering(bool _force)
{
	ovr_RequestBoundaryVisible(session, _force);
}

float OculusImpl::update_expected_frame_time()
{
	if (!is_ok())
	{
		return 1.0f / 90.0f; // fallback
	}
	update_perf_stats();
	if (perfStats.FrameStatsCount > 0)
	{
		ovrPerfStatsPerCompositorFrame& lastFramePerfStats = perfStats.FrameStats[0];
	}
	float frequency = 0.0f;
	if (frameTimeSeconds[0].is_set() && frameTimeSeconds[1].is_set())
	{
		float delta = (float)abs<double>(frameTimeSeconds[0].get() - frameTimeSeconds[1].get());
		if (delta != 0.0f)
		{
			frequency = round(1.0f / delta);
			todo_note(TXT("round to display refresh rate?"));
		}
	}
	if (frequency == 0.0f)
	{
		frequency = hmdDesc.DisplayRefreshRate;
	}
	if (frequency != 0.0f)
	{
		return 1.0f / frequency;
	}
	else
	{
		return 0.0f;
	}
}

float OculusImpl::calculate_ideal_expected_frame_time() const
{
	if (!is_ok())
	{
		return 1.0f / 90.0f; // fallback
	}
	float frequency = hmdDesc.DisplayRefreshRate;
	if (frequency != 0.0f)
	{
		return 1.0f / frequency;
	}
	else
	{
		return 0.0f;
	}
}

void OculusImpl::update_hands_controls_source()
{
	// we depend on being forced
	for_count(int, index, Hands::Count)
	{
		owner->access_controls().hands[index].source = VR::Input::Devices::find(modelNumber, trackingSystemName, String::empty());
	}
}

void OculusImpl::draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font)
{
	Vector2 at = _at;
	Vector2 triggerSize = Vector2(_width, _width * 0.1f);
	Vector2 padSize = Vector2(_width, _width);
	Vector2 separation = Vector2(_width * 0.05f, _width * 0.05f);

	Vector2 buttonSize= Vector2(_width / 8.0f, _width / 8.0f);

	int iController = handControllerTrackDevices[_hand].controllerType;
	if (iController != NONE)
	{
		int handType = controller_type_to_hand(iController);
		ovrInputState inputState;
		auto result = ovr_GetInputState(session, (ovrControllerType)iController, &inputState);
		if (OVR_SUCCESS(result))
		{
			ARRAY_STATIC(float, axes, 32);
			axes.push_back(inputState.IndexTrigger[handType]);
			axes.push_back(inputState.HandTrigger[handType]);
			for_count(int, iAxis, axes.get_size())
			{
				{
					at.y -= triggerSize.y * 0.5f;
					::System::Video3DPrimitives::fill_rect_2d(Colour::blue.with_alpha(0.3f), at - triggerSize * 0.5f, at + triggerSize * 0.5f);
					::System::Video3DPrimitives::fill_rect_2d(Colour::red.with_alpha(0.8f), at - triggerSize * 0.5f, Vector2(at.x - triggerSize.x * 0.5f + triggerSize.x * axes[iAxis], at.y + triggerSize.y * 0.5f));
					_font->draw_text_at(::System::Video3D::get(), String::printf(TXT("%i"), iAxis).to_char(), Colour::white, at, Vector2::one, Vector2::half, false);
					at.y -= triggerSize.y;
					at.y += triggerSize.y * 0.5f;
				}
				at.y -= separation.y;
			}

			{
				at.y -= padSize.y * 0.5f;
				::System::Video3DPrimitives::fill_rect_2d(Colour::blue.with_alpha(0.3f), at - padSize * 0.5f, at + padSize * 0.5f);
				auto const & joystick = get_vector_2_from(inputState.Thumbstick[handType]);
				Vector2 joyPt = Vector2(joystick.x, joystick.y);
				::System::Video3DPrimitives::fill_rect_2d(Colour::red.with_alpha(0.8f), at + joyPt * 0.48f * padSize - padSize * 0.02f, at + joyPt * 0.48f * padSize + padSize * 0.02f);
				_font->draw_text_at(::System::Video3D::get(), TXT("ts"), Colour::white, at, Vector2::one, Vector2::half, false);
				at.y -= padSize.y;
				at.y += padSize.y * 0.5f;
				at.y -= separation.y;
			}

			{
				at.y -= buttonSize.y * 0.5f;
				for_count(int, iButton, 64)
				{
					Vector2 a = at;
					a.x += ((float)(iButton % 8) - 3.5f) * buttonSize.x;
					_font->draw_text_at(::System::Video3D::get(), String::printf(TXT("%i"), iButton).to_char(), Colour::white, a, Vector2::one, Vector2::half, false);
					::System::Video3DPrimitives::fill_rect_2d(Colour::blue.with_alpha(0.3f), a - buttonSize * 0.5f, a + buttonSize * 0.5f);
					if ((inputState.Buttons >> iButton) & 1)
					{
						::System::Video3DPrimitives::fill_rect_2d(Colour::orange.with_alpha(0.3f), a - buttonSize * 0.5f, a + buttonSize * 0.5f);
					}
					if ((inputState.Buttons >> iButton) & 1)
					{
						::System::Video3DPrimitives::fill_rect_2d(Colour::red.with_alpha(0.3f), a - buttonSize * 0.5f, a + buttonSize * 0.5f);
					}
					if ((iButton % 8) == 7)
					{
						at.y -= buttonSize.y;
					}
				}
				at.y += buttonSize.y * 0.5f;
				at.y -= separation.y;
			}
		}
	}
}

void OculusImpl::update_render_scaling(REF_ float & _scale)
{
	update_perf_stats();
	float targetScale = MainConfig::global().get_video().allowAutoAdjustForVR? perfStats.AdaptiveGpuPerformanceScale : 1.0f;
	_scale = blend_to_using_speed_based_on_time(_scale, targetScale, 0.5f, ::System::Core::get_raw_delta_time());
	_scale = clamp(_scale, 0.6f, 1.0f);
}

void OculusImpl::update_perf_stats()
{
	if (perfStatsFrame != frameIndexToSubmit)
	{
		perfStatsFrame = frameIndexToSubmit;
		MEASURE_PERFORMANCE(getPerfStats);
		perfStats = {};
		ovr_GetPerfStats(session, &perfStats);
	}
}

void OculusImpl::set_render_mode(RenderMode::Type _mode)
{
	todo_note(TXT("implement various modes"));
}

tchar const* OculusImpl::get_prompt_controller_suffix(Input::DeviceID _source) const
{
	if (hmdDesc.Type == ovrHmd_CV1) return TXT("_1");

	return TXT("");
}

#endif