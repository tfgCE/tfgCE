#include "oculusMobileImpl.h"

#ifdef AN_WITH_OCULUS_MOBILE
#include "..\iVR.h"
#include "..\vrFovPort.h"

#include "..\..\mainConfig.h"

#include "..\..\math\math.h"
#include "..\..\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\mesh\mesh3d.h"
#include "..\..\splash\splashLogos.h"
#include "..\..\system\core.h"
#include "..\..\system\android\androidApp.h"
#include "..\..\system\sound\soundSystem.h"
#include "..\..\system\video\font.h"
#include "..\..\system\video\video3d.h"
#include "..\..\system\video\videoGLExtensions.h"
#include "..\..\system\video\indexFormat.h"
#include "..\..\system\video\renderTarget.h"
#include "..\..\system\video\renderTargetUtils.h"
#include "..\..\system\video\video3dPrimitives.h"

#include "..\..\containers\arrayStack.h"

#include "..\..\performance\performanceUtils.h"

#include <unistd.h>

#include "VrApi.h"
#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"
#include "VrApi_Input.h"

//

using namespace VR;

//

#ifdef AN_DEVELOPMENT
#define DEBUG_DECIDE_HANDS
#endif

//#define OWN_FOVEATED_IMPLEMENTATION
#define VRAPI_FOVEATED_IMPLEMENTATION

// only one should be active
#ifdef OWN_FOVEATED_IMPLEMENTATION
	#ifdef VRAPI_FOVEATED_IMPLEMENTATION
		#undef VRAPI_FOVEATED_IMPLEMENTATION
	#endif
#endif
#ifndef OWN_FOVEATED_IMPLEMENTATION
	#ifndef VRAPI_FOVEATED_IMPLEMENTATION
		#define VRAPI_FOVEATED_IMPLEMENTATION
	#endif
#endif

//

// check if we've got the right lib version

#if VRAPI_PRODUCT_VERSION != 1
#error invalid vrapi product version, get the right lib (code_externals) or update version here
#endif
#if VRAPI_MAJOR_VERSION != 1
#error invalid vrapi major version, get the right lib (code_externals) or update version here
#endif
#if VRAPI_MINOR_VERSION != 50
#error invalid vrapi minor version, get the right lib (code_externals) or update version here
#endif

//

static tchar const * vrModelPrefix = TXT("oculus__");

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

static Matrix44 get_matrix_from(ovrMatrix4f const & _matrix)
{
	Matrix44 m;
	m.m00 =  _matrix.M[0][0];
	m.m01 = -_matrix.M[2][0];
	m.m02 =  _matrix.M[1][0];
	m.m03 =  _matrix.M[3][0];
	m.m10 = -_matrix.M[0][2];
	m.m11 =  _matrix.M[2][2];
	m.m12 = -_matrix.M[1][2];
	m.m13 = -_matrix.M[3][2];
	m.m20 =  _matrix.M[0][1];
	m.m21 = -_matrix.M[2][1];
	m.m22 =  _matrix.M[1][1];
	m.m23 =  _matrix.M[3][1];
	m.m30 =  _matrix.M[0][3];
	m.m31 = -_matrix.M[2][3];
	m.m32 =  _matrix.M[1][3];
	m.m33 =  _matrix.M[3][3];
	return m;
}

static Transform get_transform_from(ovrMatrix4f const & _matrix)
{
	Matrix44 m = get_matrix_from(_matrix);
	return m.to_transform();
}

static bool run_ovr(tchar const * _when, ovrResult _result)
{
	if (_result >= 0) // ovrSuccess or above (boundary invalid?)
	{
		return true;
	}
	else
	{
		error(TXT("oculus mobile error (%i) on: %S"), _result, _when);
		return false;
	}
}

//

bool OculusMobileImpl::s_vrapiInitialised = false;

bool OculusMobileImpl::is_available()
{
#ifdef AN_VRAPI
	return true;
#endif
	/*
	if (s_vrapiInitialised)
	{
		// quest only atm?
		int deviceType = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_DEVICE_TYPE);
		if (deviceType >= VRAPI_DEVICE_TYPE_OCULUSQUEST_START &&
			deviceType <= VRAPI_DEVICE_TYPE_OCULUSQUEST_END)
		{
		}
	}
	*/
	return false;
}

OculusMobileImpl::OculusMobileImpl(OculusMobile* _owner)
: base(_owner)
{
}

void OculusMobileImpl::init_impl()
{
	System::Core::access_system_tags().set_tag(Name(TXT("oculusMobile")));
	Splash::Logos::add(TXT("oculusMobile"), SPLASH_LOGO_IMPORTANT);

	if (!s_vrapiInitialised)
	{
		output(TXT("initialising VRAPI..."));

		java.Vm = ::System::Android::App::get().activityVM;
		java.Env = ::System::Android::App::get().activityEnv;
		java.ActivityObject = ::System::Android::App::get().activityObject;

		const ovrInitParms initParms = vrapi_DefaultInitParms(&java);
		int32_t initResult = vrapi_Initialize(&initParms);
		if (initResult != 0)
		{
			error(TXT("error initialising VR API"));
		}
		else
		{
			s_vrapiInitialised = true;
		}
	}

	output(TXT("initialising oculus mobile"));

	trackingSystemName = String(TXT("oculusMobile"));
	modelNumber = String(TXT("OculusTouch")); // forced
}

void OculusMobileImpl::shutdown_impl()
{
	if (!::System::Core::restartRequired)
	{
		if (s_vrapiInitialised)
		{
			output(TXT("closing VRAPI..."));
			vrapi_Shutdown();
			output(TXT("closed VRAPI"));
		}
		s_vrapiInitialised = false;
	}

	ovr = nullptr;
}

void OculusMobileImpl::get_ready_to_exit_impl()
{
	int frameFlags = 0;
	frameFlags |= VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL;

	ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();
	layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

	const ovrLayerHeader2* layers[] =
	{
		&layer.Header,
	};

	ovrSubmitFrameDescription2 frameDesc = { 0 };
	frameDesc.Flags = frameFlags;
	frameDesc.SwapInterval = 1;
	frameDesc.FrameIndex = frameIndex;
	frameDesc.DisplayTime = predictedDisplayTime;
	frameDesc.LayerCount = 1;
	frameDesc.Layers = layers;

	vrapi_SubmitFrame2(ovr, &frameDesc);
}

void OculusMobileImpl::handle_vr_mode_changes_impl()
{
	scoped_call_stack_info(TXT("handle_vr_mode_changes"));
	if (!::System::Android::App::get().paused && ::System::Android::App::get().nativeWindow)
	{
		if (!ovr)
		{
			ovrModeParms parms = vrapi_DefaultModeParms(&java);
			// No need to reset the FLAG_FULLSCREEN window flag when using a View
			parms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

			parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
			parms.WindowSurface = (size_t)::System::Android::App::get().nativeWindow;
			if (auto* v3d = ::System::Video3D::get())
			{
				if (auto* eglCtx = v3d->get_gl_context())
				{
					parms.Display = (size_t)eglCtx->display;
					parms.ShareContext = (size_t)eglCtx->context;
				}
				else
				{
					an_assert(false, TXT("missing egl context!"));
				}
			}
			else
			{
				an_assert(false, TXT("missing v3d!"));
			}

			if (MainConfig::global().get_vr_mode() == VRMode::PhaseSync)
			{
				parms.Flags |= VRAPI_MODE_FLAG_PHASE_SYNC;
			}

			ovr = vrapi_EnterVrMode(&parms);

			// Set app-specific parameters once we have entered VR mode and have a valid ovrMobile.
			if (ovr)
			{
				set_processing_levels_impl();
				update_perf_threads();
				vrapi_SetTrackingSpace(ovr, VRAPI_TRACKING_SPACE_STAGE);
			}
		}
	}
	else
	{
		if (ovr)
		{
			vrapi_LeaveVrMode(ovr);
			ovr = nullptr;
		}
	}
}

void OculusMobileImpl::create_device_impl()
{
	output(TXT("create device"));
	output(TXT("device : %S"), trackingSystemName.to_char());

	// we read it although it's unlikely we will use it
	preferredFullScreenResolution.x = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_DISPLAY_PIXELS_WIDE);
	preferredFullScreenResolution.y = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_DISPLAY_PIXELS_HIGH);

	wireless = true;

	vrapi_SetTrackingSpace(ovr, VRAPI_TRACKING_SPACE_STAGE);
}

void OculusMobileImpl::enter_vr_mode_impl()
{
	// we need ovr to be present
	process_vr_events(true);
}

void OculusMobileImpl::close_device_impl()
{
	close_internal_render_targets();
}

void OculusMobileImpl::close_internal_render_targets()
{
	for_count(int, rm, InternalRenderMode::COUNT)
	{
		for_count(int, eyeIdx, 2)
		{
			auto& eye = renderInfo[rm].eye[eyeIdx];
			if (eye.colourTextureSwapChain)
			{
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDeleteFramebuffers(eye.textureSwapChainLength, eye.frameBuffers); AN_GL_CHECK_FOR_ERROR
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDeleteRenderbuffers(eye.textureSwapChainLength, eye.depthBuffers); AN_GL_CHECK_FOR_ERROR
				vrapi_DestroyTextureSwapChain(eye.colourTextureSwapChain);
				vrapi_DestroyTextureSwapChain(eye.depthTextureSwapChain);
				eye.colourTextureSwapChain = nullptr;
				eye.depthTextureSwapChain = nullptr;
				free(eye.frameBuffers);
				free(eye.depthBuffers);
				eye.frameBuffers = nullptr;
				eye.depthBuffers = nullptr;
				eye.directToVRRenderTargets.clear();
			}
		}
	}
}

bool OculusMobileImpl::can_load_play_area_rect_impl()
{
	// ovr might be null and that's fine here
	if (!ovr)
	{
		return false;
	}

	ovrPosef ovrOrigin;
	ovrVector3f ovrDimensions;
	if (vrapi_GetBoundaryOrientedBoundingBox(ovr, &ovrOrigin, &ovrDimensions) != ovrSuccess)
	{
		// it might be not possible yet!
		return false;
	}

	Vector3 dimensions = get_vector_from(ovrDimensions);
	dimensions *= 2.0f; // because oculus mobile returns values in halves

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

bool OculusMobileImpl::load_play_area_rect_impl(bool _loadAnything)
{
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));
	ovrPosef ovrOrigin;
	ovrVector3f ovrDimensions;
	if (!run_ovr(TXT("vrapi_GetBoundaryOrientedBoundingBox"), vrapi_GetBoundaryOrientedBoundingBox(ovr, &ovrOrigin, &ovrDimensions)))
	{
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

	Transform origin = get_transform_from(ovrOrigin);
	owner->set_map_vr_space_origin(origin);

#ifdef AN_INSPECT_VR_PLAY_AREA
	output(TXT("[OCULUS-MOBILE] play area centre read %.3fx%.3f (yaw %.0f)"), origin.get_translation().x, origin.get_translation().y, origin.get_orientation().to_rotator().yaw);
#endif

	// dimensions after origin

	Vector3 dimensions = get_vector_from(ovrDimensions);
	dimensions *= 2.0f; // because oculus mobile returns values in halves

#ifdef AN_INSPECT_VR_PLAY_AREA
	output(TXT("[OCULUS-MOBILE] play area size read %.3fx%.3f"), dimensions.x, dimensions.y);
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
		uint32_t count;
		if (run_ovr(TXT("vrapi_GetBoundaryGeometry"), vrapi_GetBoundaryGeometry(ovr, 0, &count, nullptr)))
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
				allocate_stack_var(ovrVector3f, points, count);
				uint32_t temp;
				if (run_ovr(TXT("vrapi_GetBoundaryGeometry"), vrapi_GetBoundaryGeometry(ovr, count, &temp, points)))
				{
					for_count(int, i, count)
					{
						boundaryPoints.push_back(get_vector_from(points[i]).to_vector2());
					}
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

	output(TXT("[ovrmob] origin: at %.3fx%.3f (+%.3f) (y:%.3f)"), origin.get_translation().x, origin.get_translation().y, origin.get_translation().z, origin.get_orientation().to_rotator().yaw);
	output(TXT("[ovrmob] half right %.3f (full %.3f)"), owner->get_raw_whole_play_area_rect_half_right().length(), dimensions.x);
	output(TXT("[ovrmob] half forward %.3f (full %.3f)"), owner->get_raw_whole_play_area_rect_half_forward().length(), dimensions.y);

	owner->act_on_possibly_invalid_boundary();

	if (owner->get_raw_whole_play_area_rect_half_right().length() >= owner->get_raw_whole_play_area_rect_half_forward().length())
	{
		return true;
	}

	return false;
}

void OculusMobileImpl::init_hand_controller_track_device(int _index)
{
	if (handControllerTrackDevices[_index].deviceID == NONE)
	{
		return;
	}

	String modelName;// = get_string_tracked_device_property(openvrSystem, handControllerTrackDevices[_index].deviceID, vr::Prop_RenderModelName_String);
	output(TXT("model : "), modelName.to_char());

	if (! modelName.is_empty())
	{
		owner->set_model_name_for_hand(_index, Name(String::printf(TXT("%S%S"), vrModelPrefix, modelName.to_char())));
	}

	if (handControllerTrackDevices[_index].controllerType == ovrControllerType_Hand)
	{
		ovrHandSkeleton skeleton;

		skeleton.Header.Version = ovrHandVersion_1;
		if (run_ovr(TXT("vrapi_GetHandSkeleton"), vrapi_GetHandSkeleton(ovr, handControllerTrackDevices[_index].hand == Hand::Left ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT, &skeleton.Header)))
		{
			auto& refHandPose = owner->access_reference_pose_set().hands[_index];
			for_count(int, b, VRHandPose::MAX_RAW_BONES)
			{
				refHandPose.rawBonesLS[b].clear();
				refHandPose.rawBonesRS[b].clear();
				refHandPose.rawBoneParents[b] = NONE;
			}
			int numBones = skeleton.NumBones;
			if (numBones > VRHandPose::MAX_RAW_BONES)
			{
				error(TXT("too many bones for a hand, this won't work right"));
				numBones = VRHandPose::MAX_RAW_BONES;
			}
			for_count(int, b, numBones)
			{
				refHandPose.rawBonesLS[b] = get_transform_from(skeleton.BonePoses[b]);
				refHandPose.rawBoneParents[b] = skeleton.BoneParentIndices[b];
			}

			// provide any placements so we may calculate stuff inside - required for reference pose
			refHandPose.rawPlacement = Transform::identity;
			refHandPose.rawHandTrackingRootPlacement = Transform::identity;

			refHandPose.calculate_auto_and_add_offsets(handControllerTrackDevices[_index].hand, true);
		}
		else
		{
			error(TXT("could not get hand's skeleton bones"));
		}
	}
}

void OculusMobileImpl::close_hand_controller_track_device(int _index)
{
	owner->set_model_name_for_hand(_index, Name::invalid());
}

bool OculusMobileImpl::check_if_requires_to_update_track_device_indices() const
{
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));
	int currentHandControllerTrackDevices[Hands::Count];
	for_count(int, tryIdx, 3)
	{
		bool lookForRemotes = tryIdx == 0 || tryIdx == 2;
		bool lookForHands = tryIdx == 1 || tryIdx == 2;

		int handIndex = 0;
		for_count(int, i, Hands::Count)
		{
			currentHandControllerTrackDevices[i] = NONE;
		}
		int i = 0;
		while (true)
		{
			ovrInputCapabilityHeader cap;
			ovrResult result = vrapi_EnumerateInputDevices(ovr, i, &cap);
			if (result < 0)
			{
				break;
			}

			if ((lookForRemotes && cap.Type == ovrControllerType_TrackedRemote) ||
				(lookForHands && cap.Type == ovrControllerType_Hand))
			{
				currentHandControllerTrackDevices[handIndex] = cap.Type;
				++handIndex;
				if (handIndex >= 2)
				{
					break;
				}
			}

			++i;
		}
		if (handIndex >= 2)
		{
			break;
		}
	}

	for (int i = 0; i < Hands::Count; ++i)
	{
		if (handControllerTrackDevices[i].controllerType != currentHandControllerTrackDevices[i])
		{
			return true;
		}
	}
	return false;
}

void OculusMobileImpl::update_track_device_indices()
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

	// close and clear
	for_count(int, i, Hands::Count)
	{
		close_hand_controller_track_device(i);
		handControllerTrackDevices[i].clear();
		owner->access_controls_pose_set().hands[i].clear();
		owner->access_render_pose_set().hands[i].clear();
		owner->access_reference_pose_set().hands[i].clear();
	}

	if (!ovr)
	{
		// won't get anything anyway
		return;
	}

	for_count(int, tryIdx, 3)
	{
		bool lookForRemotes = tryIdx == 0 || tryIdx == 2;
		bool lookForHands = tryIdx == 1 || tryIdx == 2;

		int handIndex = 0;
		for_count(int, i, Hands::Count)
		{
			handControllerTrackDevices[i].controllerType = NONE;
		}
		int i = 0;
		while (true)
		{
			ovrInputCapabilityHeader cap;
			ovrResult result = vrapi_EnumerateInputDevices(ovr, i, &cap);
			if (result < 0)
			{
				break;
			}

			if ((lookForRemotes && cap.Type == ovrControllerType_TrackedRemote) ||
				(lookForHands && cap.Type == ovrControllerType_Hand))
			{
				handControllerTrackDevices[handIndex].controllerType = cap.Type;
				handControllerTrackDevices[handIndex].deviceID = cap.DeviceID;
				handControllerTrackDevices[handIndex].hand = Hand::MAX;

				if (cap.Type == ovrControllerType_TrackedRemote)
				{
					ovrInputTrackedRemoteCapabilities remoteCaps;
					remoteCaps.Header = cap;

					if (run_ovr(TXT("vrapi_GetInputDeviceCapabilities"), vrapi_GetInputDeviceCapabilities(ovr, &remoteCaps.Header)))
					{
						if (remoteCaps.ControllerCapabilities & ovrControllerCaps_LeftHand)
						{
							handControllerTrackDevices[handIndex].hand = Hand::Left;
						}
						if (remoteCaps.ControllerCapabilities & ovrControllerCaps_RightHand)
						{
							handControllerTrackDevices[handIndex].hand = Hand::Right;
						}
					}
				}

				if (cap.Type == ovrControllerType_Hand)
				{
					ovrInputHandCapabilities handCaps;
					handCaps.Header = cap;
					if (run_ovr(TXT("vrapi_GetInputDeviceCapabilities"), vrapi_GetInputDeviceCapabilities(ovr, &handCaps.Header)))
					{
						if (handCaps.HandCapabilities & ovrHandCaps_LeftHand)
						{
							handControllerTrackDevices[handIndex].hand = Hand::Left;
						}
						if (handCaps.HandCapabilities & ovrHandCaps_RightHand)
						{
							handControllerTrackDevices[handIndex].hand = Hand::Right;
						}
					}
				}

				++handIndex;
				if (handIndex >= Hands::Count)
				{
					break;
				}
			}

			++i;
		}
		if (handIndex >= Hands::Count)
		{
			break;
		}
	}

	{	// decide whether we're using hand tracking or not, do it before init, so we get lengths, base-to-tip etc.
		for_count(int, handIndex, Hands::Count)
		{
			owner->set_using_hand_tracking(handIndex, handControllerTrackDevices[handIndex].controllerType == ovrControllerType_Hand);
		}
	}

	// we need to decide hands here as well, as calculations may require accessing the right hands
	// this will be done "by system" as we know which hand's left and right
	// also here we update input
	owner->decide_hands();

	// init all
	for_count(int, handIndex, Hands::Count)
	{
		if (handControllerTrackDevices[handIndex].controllerType != NONE)
		{
			init_hand_controller_track_device(handIndex);
		}
	}

	owner->mark_new_controllers();
}

void OculusMobileImpl::store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, ovrTracking2 const & _trackingState, ovrTracking const * _handsTrackingState)
{
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));

	if (_trackingState.Status & (VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED | VRAPI_TRACKING_STATUS_POSITION_TRACKED))
	{
		_poseSet.rawViewAsRead = get_transform_from(_trackingState.HeadPose.Pose).to_world(viewInHeadPoseSpace);
	}
#ifdef AN_INSPECT_VR_INVALID_ORIGIN_CONTINUOUS
	else
	{
		output(TXT("[vr_invalid_origin] no head provided"));
	}
#endif

	for_count(int, i, Hands::Count)
	{
		int iController = handControllerTrackDevices[i].controllerType;
		Hand::Type handType = handControllerTrackDevices[i].hand;
		if (iController != NONE)
		{
			auto & pose = _poseSet.hands[i];
			if (_handsTrackingState[i].Status & (VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED | VRAPI_TRACKING_STATUS_POSITION_TRACKED))
			{
				Transform vrHandPose = get_transform_from(_handsTrackingState[i].HeadPose.Pose);
				Transform vrHandRoot = vrHandPose;
				Transform vrHandPointer = vrHandPose;
				bool writeVRHandRoot = true;
				bool writeVRHandPointer = false;
				if (handControllerTrackDevices[i].controllerType == ovrControllerType_Hand)
				{
					if (_controls)
					{
						auto& controls = _controls->hands[i];
						// zero everything, we'll fill pinch if required
						controls.trackpadTouchDelta = Vector2::zero; // reset delta, we will update it if possible

						controls.upsideDown = 0.0f;
						controls.insideToHead = 0.0f;
						controls.pointerPinch = 0.0f;
						controls.middlePinch = 0.0f;
						controls.ringPinch = 0.0f;
						controls.pinkyPinch = 0.0f;
						controls.trigger = 0.0f;
						controls.grip = 0.0f;
						controls.pose = 0.0f;
						controls.thumb = 0.0f;
						controls.pointer = 0.0f;
						controls.middle = 0.0f;
						controls.ring = 0.0f;
						controls.pinky = 0.0f;

						controls.joystick = Vector2::zero;

						controls.buttons.primary = false;
						controls.buttons.secondary = false;
						controls.buttons.joystickPress = false;
					}

					{
						ovrInputStateHand handInputState;
						handInputState.Header.ControllerType = (ovrControllerType)handControllerTrackDevices[i].controllerType;
						if (run_ovr(TXT("vrapi_GetCurrentInputState"), vrapi_GetCurrentInputState(ovr, handControllerTrackDevices[i].deviceID, &handInputState.Header)))
						{
							vrHandPointer = get_transform_from(handInputState.PointerPose);
							writeVRHandPointer = true;
							if (_controls)
							{
								auto& controls = _controls->hands[i];
								controls.pointerPinch = (handInputState.InputStateStatus & ovrInputStateHandStatus_IndexPinching)? handInputState.PinchStrength[ovrHandPinchStrength_Index] : 0.0f;
								controls.middlePinch = (handInputState.InputStateStatus & ovrInputStateHandStatus_MiddlePinching) ? handInputState.PinchStrength[ovrHandPinchStrength_Middle] : 0.0f;
								controls.ringPinch = (handInputState.InputStateStatus & ovrInputStateHandStatus_RingPinching) ? handInputState.PinchStrength[ovrHandPinchStrength_Ring] : 0.0f;
								controls.pinkyPinch = (handInputState.InputStateStatus & ovrInputStateHandStatus_PinkyPinching) ? handInputState.PinchStrength[ovrHandPinchStrength_Pinky] : 0.0f;
								controls.buttons.systemGestureProcessing = (handInputState.InputStateStatus & ovrInputStateHandStatus_SystemGestureProcessing);
								if (owner->get_dominant_hand() != handType)
								{
									controls.buttons.systemMenu = (handInputState.InputStateStatus & ovrInputStateHandStatus_MenuPressed);
								}
								else
								{
									controls.buttons.systemMenu = false;
								}
							}
						}
					}

					if (_controls)
					{
						auto& controls = _controls->hands[i];
						controls.do_auto_buttons();
					}

					{
						{
							// keep as it was by default
							writeVRHandRoot = false;
						}
						ovrHandPose currentHandPose;
						currentHandPose.Header.Version = ovrHandVersion_1;
						if (run_ovr(TXT("vrapi_GetHandPose"), vrapi_GetHandPose(ovr, handControllerTrackDevices[i].deviceID, 0.0f, &(currentHandPose.Header))))
						{
							if (currentHandPose.Status == ovrHandTrackingStatus_Tracked)
							{
								// read bones inside
								auto const& refPose = owner->get_reference_pose_set().hands[i];
								for_count(int, b, VRHandPose::MAX_RAW_BONES)
								{
									pose.rawBonesRS[b].clear();
									auto const& refBoneLS = refPose.rawBonesLS[b];
									if (refBoneLS.is_set())
									{
										auto& boneLS = pose.rawBonesLS[b];
										boneLS = refBoneLS;
										boneLS.access().set_orientation(get_quat_from(currentHandPose.BoneRotations[b]));
									}
								}

								vrHandRoot = get_transform_from(currentHandPose.RootPose);

								writeVRHandRoot = true;
							}
						}
					}
				}
				else
				{
					if (_controls)
					{
						auto& controls = _controls->hands[i];
						controls.trackpadTouchDelta = Vector2::zero; // reset delta, we will update it if possible
						ovrInputStateTrackedRemote inputState;
						inputState.Header.ControllerType = (ovrControllerType)iController;
						if (run_ovr(TXT("ovr_GetInputState"), vrapi_GetCurrentInputState(ovr, handControllerTrackDevices[i].deviceID, &inputState.Header)))
						{
							controls.upsideDown.clear();
							controls.insideToHead.clear();
							controls.pointerPinch.clear();
							controls.middlePinch.clear();
							controls.ringPinch.clear();
							controls.pinkyPinch.clear();
							controls.trigger = inputState.IndexTrigger;
							int indexPointing = ovrTouch_IndexPointing;
							int thumpUp = ovrTouch_ThumbUp;
							if ((inputState.Touches & indexPointing) && inputState.GripTrigger > VRControls::AXIS_TO_BUTTON_THRESHOLD)
							{
								controls.pose = 1.0f;
								controls.thumb = (inputState.Touches & thumpUp)? 0.0f : 1.0f;
								controls.pointer = 0.0f;
								controls.middle = 1.0f;
								controls.ring = 1.0f;
								controls.pinky = 1.0f;
								controls.grip = 1.0f;
							}
							else if ((inputState.Touches & thumpUp) && inputState.GripTrigger > VRControls::AXIS_TO_BUTTON_THRESHOLD)
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
								controls.thumb = inputState.GripTrigger;
								controls.pointer = inputState.GripTrigger;
								controls.middle = inputState.GripTrigger;
								controls.ring = inputState.GripTrigger;
								controls.pinky = inputState.GripTrigger;
								controls.grip = inputState.GripTrigger;
							}

							controls.joystick = get_vector_2_from(inputState.Joystick);

							controls.do_auto_buttons();

							if (handType == Hand::Right)
							{
								controls.buttons.primary = (inputState.Buttons & ovrButton_A) != 0;
								controls.buttons.secondary = (inputState.Buttons & ovrButton_B) != 0;
								controls.buttons.joystickPress = (inputState.Buttons & ovrButton_RThumb) != 0;
							}
							else if (handType == Hand::Left)
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
							controls.buttons.systemGestureProcessing = false;
						}
					}
				}

				pose.rawPlacementAsRead = vrHandPose;
				if (writeVRHandRoot)
				{
					pose.rawHandTrackingRootPlacementAsRead = vrHandRoot;
				}
				else
				{
					pose.rawHandTrackingRootPlacementAsRead.clear();
				}
			}
		}
		else
		{
			_poseSet.hands[i].rawPlacementAsRead.clear();
			_poseSet.hands[i].rawHandTrackingRootPlacementAsRead.clear();
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

void OculusMobileImpl::process_vr_events(bool _requiresToHaveOVR)
{
	auto& questApp = System::Android::App::get();
	// process all events once
	{
		scoped_call_stack_info(TXT("process events, quickly"));
		questApp.process_events(! ovr);
	}
	if (_requiresToHaveOVR)
	{
		scoped_call_stack_info(TXT("requires to have OVR"));
		// wait for Quest to be put on a head
		while (!ovr)
		{
			scoped_call_stack_info(TXT("process events until has them"));
			questApp.process_events(true);
		}
	}
}

void OculusMobileImpl::advance_events()
{
	scoped_call_stack_info(TXT("oculus mobile advance events"));
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));

	{
		scoped_call_stack_info(TXT("process vr events"));
		//System::FaultHandler::disable();
		process_vr_events(true);
		//System::FaultHandler::enable();
	}

	{
		todo_note(TXT("implement!"));
		/*
		ovrSessionStatus sessionStatus;
		run_ovr(TXT("ovr_GetSessionStatus"), ovr_GetSessionStatus(ovr, &sessionStatus));
		if (sessionStatus.ShouldQuit)
		{
			System::Core::quick_exit(true);
		}
		*/
	}

	// update dominant hand
	{
		if (vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_DOMINANT_HAND) == VRAPI_HAND_LEFT)
		{
			owner->set_dominant_hand(Hand::Left);
		}
		else
		{
			owner->set_dominant_hand(Hand::Right);
		}
	}

	{
		scoped_call_stack_info(TXT("check for track devices"));
		if (check_if_requires_to_update_track_device_indices())
		{
			scoped_call_stack_info(TXT("devices changed"));
			output(TXT("devices changed!"));
			update_track_device_indices();
			owner->decide_hands();
		}
	}

	{
		int newRecenterCount = vrapi_GetSystemStatusInt(&java, VRAPI_SYS_STATUS_USER_RECENTER_COUNT);
		if (recenterCount != newRecenterCount)
		{
			recenterCount = newRecenterCount;
			owner->access_controls().requestRecenter = true;
			owner->reset_immobile_origin_in_vr_space();
		}
	}

	// EVENTS!
	{
		ovrEventDataBuffer eventDataBuffer = {};

		while (true)
		{
			ovrEventHeader* eventHeader = (ovrEventHeader*)(&eventDataBuffer);
			ovrResult res = vrapi_PollEvent(eventHeader);
			if (res != ovrSuccess)
			{
				break;
			}

			switch (eventHeader->EventType)
			{
			case VRAPI_EVENT_VISIBILITY_GAINED:
				output(TXT("[system] visibility gained"));
				::System::Core::unpause_rendering(bit(2));
				::System::Core::unpause_vr(bit(2));
				break;
			case VRAPI_EVENT_VISIBILITY_LOST:
				output(TXT("[system] visibility lost"));
				::System::Core::pause_rendering(bit(2));
				::System::Core::pause_vr(bit(2));
				break;
			case VRAPI_EVENT_FOCUS_GAINED:
				output(TXT("[system] focus gained"));
				::System::Core::unpause_vr(bit(1));
				break;
			case VRAPI_EVENT_FOCUS_LOST:
				output(TXT("[system] focus lost"));
				::System::Core::pause_vr(bit(1));
				break;
			default:
				break;
			}
		}
	}
}

void OculusMobileImpl::advance_pulses()
{
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));

	{
		int handIdx = 0;
		while (auto* pulse = owner->get_pulse(handIdx))
		{
			float currentStrength = pulse->get_current_strength() * MainConfig::global().get_vr_haptic_feedback();
			if (currentStrength > 0.0f)
			{
				vrapi_SetHapticVibrationSimple(ovr, (ovrControllerType)handControllerTrackDevices[handIdx].deviceID, currentStrength);
			}
			++handIdx;
		}
	}
}

void OculusMobileImpl::advance_poses()
{
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));

	auto & vrControls = owner->access_controls();
	auto & poseSet = owner->access_controls_pose_set();

	// get poses (do not predict tracking as then app will thing we're behind with frames
	ovrTracking2 const trackingState = vrapi_GetPredictedTracking2(ovr, predictedDisplayTime);
	ovrTracking handsTrackingState[2];
	for_count(int, i, Hands::Count)
	{
		handsTrackingState[i].Status = 0;
		if (handControllerTrackDevices[i].deviceID != NONE)
		{
			vrapi_GetInputTrackingState(ovr, handControllerTrackDevices[i].deviceID, 0.0f, &handsTrackingState[i]);
		}
	}
	store(poseSet, &vrControls, trackingState, handsTrackingState);
}

void OculusMobileImpl::next_frame()
{
	if (!is_ok())
	{
		return;
	}
	++frameIndex;
}

void OculusMobileImpl::game_sync_impl()
{
	if (MainConfig::global().get_vr_mode() == VRMode::PhaseSync)
	{
		vrapi_WaitFrame(ovr, frameIndex);
		PERFORMANCE_MARKER(Colour::green);
		syncPredictedDisplayTime = vrapi_GetPredictedDisplayTime(ovr, frameIndex);
		double vrSyncDeltaTime = syncPredictedDisplayTime - prevSyncPredictedDisplayTime.get(syncPredictedDisplayTime);
		prevSyncPredictedDisplayTime = syncPredictedDisplayTime;
		::System::Core::force_vr_sync_delta_time(vrSyncDeltaTime);
		// we may now allow vrapi_BeginFrame
		beginFrameGate.allow_one();
	}
}

bool OculusMobileImpl::begin_render(System::Video3D * _v3d)
{
	if (!is_ok())
	{
		return false;
	}
	if (MainConfig::global().get_vr_mode() == VRMode::PhaseSync)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!beginFrameGate.wait_for(1.0f))
		{
			error(TXT("waiting too long for frame to begin, maybe game_sync was not called?"));
		}
#else
		beginFrameGate.wait();
#endif
		vrapi_BeginFrame(ovr, frameIndex);
	}
	frameIndexToSubmit = frameIndex; // store which frame are we going to submit on end_render
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));
	if (renderMode != pendingRenderMode)
	{
		renderMode = pendingRenderMode; // switch render modes to render in a different resolution
		/*
		if (usingFoveatedRendering)
		{
			int level = TypeConversions 4.0f * MainConfig::global().get_video().vrFoveatedRenderingLevel;
			if (renderMode == InternalRenderMode::HiRes)
			{
				level = max(0, level - 1);
			}
			vrapi_SetPropertyInt(&java, VRAPI_FOVEATION_LEVEL, level);
		}
		*/
	}
	if (::System::FoveatedRenderingSetup::should_force_set_foveation())
	{
		if (usingFoveatedRendering)
		{
			auto vraf = get_available_functionality();
			int level = vraf.convert_foveated_rendering_config_to_level(MainConfig::global().get_video().vrFoveatedRenderingLevel);
			vrapi_SetPropertyInt(&java, VRAPI_FOVEATION_LEVEL, level);
		}
	}
	for_count(int, eyeIdx, 2)
	{
		auto& eye = access_oculus_render_info().eye[eyeIdx];
		eye.textureSwapChainIndex = frameIndexToSubmit % eye.textureSwapChainLength;
	}
	// get poses to render - 
#ifdef AN_OUTPUT_DETAILED_FRAMES
	lastPredictedDisplayTime = predictedDisplayTime;
	double prevPredictedAtTime = predictedAtTime;
	predictedAtTime = vrapi_GetTimeInSeconds();
#endif
	predictedDisplayTime = vrapi_GetPredictedDisplayTime(ovr, frameIndexToSubmit);
#ifdef AN_OUTPUT_DETAILED_FRAMES
	output(TXT(" ======================="));
	output(TXT(" frame %05i"), frameIndexToSubmit);
	output(TXT(" previous predicted at          %15.5f"), prevPredictedAtTime);
	output(TXT(" now predicted at               %15.5f      diff = %15.5f"), predictedAtTime, predictedAtTime - prevPredictedAtTime);
	output(TXT(" last predicted display time    %15.5f"), lastPredictedDisplayTime);
	output(TXT(" now predicted display time     %15.5f      diff = %15.5f %S"), predictedDisplayTime, predictedDisplayTime - lastPredictedDisplayTime, predictedDisplayTime - lastPredictedDisplayTime < 0.012 || predictedDisplayTime - lastPredictedDisplayTime > 0.15? TXT("<-----------") : TXT(""));
	output(TXT(" to next display time           %15.5f"), predictedDisplayTime - predictedAtTime);
#endif
	ovrTracking2 const trackingState = vrapi_GetPredictedTracking2(ovr, predictedDisplayTime);
	renderPoseSetTimeSeconds = vrapi_GetTimeInSeconds();
	// store head pose for which we rendered
	trackingStateForRendering = trackingState;
	// hands
	ovrTracking handsTrackingState[2];
	for_count(int, i, Hands::Count)
	{
		handsTrackingState[i].Status = 0;
		if (handControllerTrackDevices[i].deviceID != NONE)
		{
			vrapi_GetInputTrackingState(ovr, handControllerTrackDevices[i].deviceID, predictedDisplayTime, &handsTrackingState[i]);
		}
	}
	store(owner->access_render_pose_set(), nullptr, trackingState, handsTrackingState);
	owner->store_last_valid_render_pose_set();
	return true;
}

bool OculusMobileImpl::end_render(System::Video3D * _v3d, ::System::RenderTargetPtr* _eyeRenderTargets)
{
	if (!is_ok())
	{
		return false;
	}
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));

	::System::RenderTarget::unbind_to_none();

	for_count(int, eyeIdx, 2)
	{
		auto& eye = access_oculus_render_info().eye[eyeIdx];

		an_assert(eye.textureSwapChainIndex == (frameIndexToSubmit % eye.textureSwapChainLength), TXT("textureSwapChainIndex should be set during begin_render and not altered!"));

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
		::System::FoveatedRenderingSetup::handle_rendering_test_grid(eyeIdx);
#endif

		if (!_eyeRenderTargets[eyeIdx].is_set())
		{
			an_assert(IVR::get()->does_use_direct_to_vr_rendering());
		}
		else
		{
			an_assert(! IVR::get()->does_use_direct_to_vr_rendering());
			/*
			 *	This is a bit nasty.
			 *	We have our own render targets that are output from the game and currently we render them onto textures provided by Oculus
			 *	In the end it would be nice to replace this with set of render targets that were created using textures provided by Oculus
			 */
			todo_note(TXT("replace this copying with proper render targets based on textures provided by Oculus?"));
			{
				MEASURE_PERFORMANCE(copyTexture);

				// resolve first
				_eyeRenderTargets[eyeIdx]->resolve_forced_unbind(); AN_GL_CHECK_FOR_ERROR
					
				const unsigned int colourTextureId = vrapi_GetTextureSwapChainHandle(eye.colourTextureSwapChain, eye.textureSwapChainIndex);

				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, eye.frameBuffers[eye.textureSwapChainIndex]); AN_GL_CHECK_FOR_ERROR
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

				VectorInt2 renderSize = access_oculus_render_info().eye[eyeIdx].frameBufferSize;

				_v3d->set_viewport(VectorInt2::zero, renderSize);
				_v3d->set_near_far_plane(0.02f, 100.0f);

				_v3d->set_2d_projection_matrix_ortho();
				_v3d->access_model_view_matrix_stack().clear();

				_eyeRenderTargets[eyeIdx]->render(0, _v3d, Vector2::zero, renderSize.to_vector2()); AN_GL_CHECK_FOR_ERROR

				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, eye.frameBuffers[eye.textureSwapChainIndex]); AN_GL_CHECK_FOR_ERROR
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0); AN_GL_CHECK_FOR_ERROR
			}

			todo_note(TXT("copy depth buffer"));
		}
	}

	::System::Video3D::get()->invalidate_bound_frame_buffer_info();
	::System::RenderTarget::unbind_to_none();

	{
		MEASURE_PERFORMANCE(flush);
		DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR
	}

	{	// store info about buffers to use and submit frames
		ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
		layer.HeadPose = trackingStateForRendering.HeadPose;
		for_count(int, eyeIdx, 2)
		{
			auto& eye = access_oculus_render_info().eye[eyeIdx];

			layer.Textures[eyeIdx].ColorSwapChain = eye.colourTextureSwapChain;
			layer.Textures[eyeIdx].SwapChainIndex = eye.textureSwapChainIndex;
			layer.Textures[eyeIdx].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&trackingStateForRendering.Eye[eyeIdx].ProjectionMatrix);
		}

		const ovrLayerHeader2* layerHeaders[] =
		{
			&layer.Header
		};

		ovrSubmitFrameDescription2 frameDesc = { 0 };
		frameDesc.Flags = 0;
		frameDesc.SwapInterval = 1;
		frameDesc.FrameIndex = frameIndexToSubmit;
		frameDesc.DisplayTime = predictedDisplayTime;
		frameDesc.LayerCount = 1;
		frameDesc.Layers = layerHeaders;

#ifdef AN_OUTPUT_DETAILED_FRAMES
		double curr = vrapi_GetTimeInSeconds();
		output(TXT("[oculusDetailedFrames] -----------------------"));
		output(TXT("[oculusDetailedFrames] since prediction to now        %15.5f"), curr - predictedAtTime);
		output(TXT("[oculusDetailedFrames] time to predicted display time %15.5f"), predictedDisplayTime - curr);
		output(TXT("[oculusDetailedFrames] submitting at                  %15.5f"), vrapi_GetTimeInSeconds());
#endif

		// Hand over the eye images to the time warp.
		{
			MEASURE_PERFORMANCE(vrapi_SubmitFrame2);
			run_ovr(TXT("vrapi_SubmitFrame2"), vrapi_SubmitFrame2(ovr, &frameDesc));
			PERFORMANCE_MARKER(Colour::red);
		}

#ifdef AN_OUTPUT_DETAILED_FRAMES
		double post = vrapi_GetTimeInSeconds();
		output(TXT("[oculusDetailedFrames] submision time                 %15.5f %S"), post - curr, post - curr > 0.013? TXT("longer submit time") : TXT(""));
		output(TXT("[oculusDetailedFrames] ======================="));
#endif
	}

	// update frame time seconds to get proper expected frame rate!
	frameTimeSeconds[1] = frameTimeSeconds[0];
	frameTimeSeconds[0] = vrapi_GetTimeInSeconds();

	return true;
}

AvailableFunctionality OculusMobileImpl::get_available_functionality() const
{
	AvailableFunctionality vraf;
	vraf.renderToScreen = false;
	vraf.directToVR = true;
	vraf.postProcess = false;
	if (s_vrapiInitialised)
	{
		if (vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_FOVEATION_AVAILABLE) == VRAPI_TRUE)
		{
			//output(TXT("[vrapi] foveated rendering available"));
#ifdef OWN_FOVEATED_IMPLEMENTATION
			vraf.foveatedRenderingNominal = ::System::FoveatedRenderingSetup::get_nominal_setup();
			vraf.foveatedRenderingMax = ::System::FoveatedRenderingSetup::get_max_setup();
			vraf.foveatedRenderingMaxDepth = 5;
#else
#ifdef VRAPI_FOVEATED_IMPLEMENTATION
			vraf.foveatedRenderingNominal = hardcoded 4;
			vraf.foveatedRenderingMax = vraf.foveatedRenderingNominal;
			vraf.foveatedRenderingMaxDepth = 0;
#else
			vraf.foveatedRenderingNominal = 0;
			vraf.foveatedRenderingMax = 0;
			vraf.foveatedRenderingMaxDepth = 0;
#endif
#endif
		}
		else
		{
			vraf.foveatedRenderingNominal = 0;
			vraf.foveatedRenderingMax = 0;
			vraf.foveatedRenderingMaxDepth = 0;
			//output(TXT("[vrapi] foveated rendering not available"));
		}
	}
	else
	{
		vraf.foveatedRenderingNominal = 0;
		vraf.foveatedRenderingMax = 0;
		vraf.foveatedRenderingMaxDepth = 0;
		//output(TXT("[vrapi] vrapi not initialised"));
	}
	return vraf;
}

void OculusMobileImpl::create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget)
{
	close_internal_render_targets();

	output(TXT("[vrapi] create_internal_render_targets"));

	//REMOVE_OR_CHANGE_BEFORE_COMMIT_ MainConfig::access_global().access_video().directlyToVR = true;
	//REMOVE_OR_CHANGE_BEFORE_COMMIT_ MainConfig::access_global().access_video().msaa = true;
	//REMOVE_OR_CHANGE_BEFORE_COMMIT_ MainConfig::access_global().access_video().vrFoveatedRenderingLevel = 1.0f;
	_useDirectToVRRendering = MainConfig::global().get_video().directlyToVR;
	_noOutputRenderTarget = true;

	// before swap chains!
	{
		auto vraf = get_available_functionality();
		output(TXT("[vrapi] vraf.foveatedRenderingNominal %i"), vraf.foveatedRenderingNominal);
		output(TXT("[vrapi] _useDirectToVRRendering %S"), _useDirectToVRRendering? TXT("yes") : TXT("no"));
#ifdef OWN_FOVEATED_IMPLEMENTATION
		if (vraf.foveatedRenderingNominal > 0 && ::System::VideoGLExtensions::get().QCOM_texture_foveated)
		{
			usingFoveatedRendering = ::System::FoveatedRenderingSetup::should_use();
		}
		else
		{
			usingFoveatedRendering = false;
		}
#endif
#ifdef VRAPI_FOVEATED_IMPLEMENTATION
		if (vraf.foveatedRenderingNominal > 0 && _useDirectToVRRendering)
		{
			int foveatedRenderingLevel = vraf.convert_foveated_rendering_config_to_level(MainConfig::global().get_video().vrFoveatedRenderingLevel);
			output(TXT("[vrapi] foveation on (%i)"), foveatedRenderingLevel);
			vrapi_SetPropertyInt(&java, VRAPI_FOVEATION_LEVEL, foveatedRenderingLevel);
			usingFoveatedRendering = true;
		}
		else
		{
			output(TXT("[vrapi] foveation off"));
			vrapi_SetPropertyInt(&java, VRAPI_FOVEATION_LEVEL, 0);
			usingFoveatedRendering = false;
		}
#endif
		if (MainConfig::global().get_vr_mode() == VRMode::ExtraLatency)
		{
			// smoother but with an extra latency
			vrapi_SetExtraLatencyMode(ovr, ovrExtraLatencyMode::VRAPI_EXTRA_LATENCY_MODE_ON);
		}
		else
		{
			vrapi_SetExtraLatencyMode(ovr, ovrExtraLatencyMode::VRAPI_EXTRA_LATENCY_MODE_OFF);
		}
	}

	for_count(int, rm, InternalRenderMode::COUNT)
	{
		for_count(int, eyeIdx, 2)
		{
			// per eye as resolution may be different for both eyes, it is not right now but who knows
			::System::RenderTargetSetup useSetup = _source;
			if (_useDirectToVRRendering)
			{
				useSetup.set_msaa_samples(MainConfig::global().get_video().get_vr_msaa_samples());
				useSetup.set_depth_stencil_format(::System::DepthStencilFormat::defaultFormat);
			}

			auto& eye = renderInfo[rm].eye[eyeIdx];
			eye.frameBufferSize = useSetup.get_size();

			{	// calculate resolution/size
				VectorInt2 resolution = eye.frameBufferSize;
				if (resolution.is_zero())
				{
					resolution = renderInfo[rm].eyeResolution[eyeIdx];
				}
				if (resolution.is_zero())
				{
					resolution = _fallbackEyeResolution * renderInfo[rm].renderScale[eyeIdx];
				}
				float useVRResolutionCoef = MainConfig::global().get_video().vrResolutionCoef;
				if (rm == InternalRenderMode::HiRes)
				{
					useVRResolutionCoef += 0.1f;
				}
				resolution *= useVRResolutionCoef * MainConfig::global().get_video().overall_vrResolutionCoef; // gl draw buffers? || what, what do you mean "gl draw buffers"?
				resolution = apply_aspect_ratio_coef(resolution, MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef);
				useSetup.set_size(resolution);
			}

			eye.frameBufferSize = useSetup.get_size();

			eye.colourTextureSwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, GL_RGBA8, useSetup.get_size().x, useSetup.get_size().y, 1, 3);
			eye.textureSwapChainLength = vrapi_GetTextureSwapChainLength(eye.colourTextureSwapChain);
			eye.frameBuffers = (GLuint*)malloc(eye.textureSwapChainLength * sizeof(GLuint));
			eye.depthBuffers = (GLuint*)malloc(eye.textureSwapChainLength * sizeof(GLuint));
			if (_useDirectToVRRendering)
			{
				eye.directToVRRenderTargets.set_size(eye.textureSwapChainLength);
			}

#ifdef AN_GLES
			auto& glExtensions = ::System::VideoGLExtensions::get();
			if (useSetup.should_use_msaa() &&
				(!glExtensions.glFramebufferTexture2DMultisampleEXT ||
				 !glExtensions.glRenderbufferStorageMultisampleEXT))
			{
				error(TXT("no gl multisample functions available, not using multisample with directly to vr"));
				useSetup.dont_use_msaa();
			}
#endif

			GLenum frameBufferTarget = useSetup.should_use_msaa()? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;

			for (int i = 0; i < eye.textureSwapChainLength; i++)
			{
				// Create the color buffer texture.
				const GLuint colourTexture = vrapi_GetTextureSwapChainHandle(eye.colourTextureSwapChain, i);
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

				DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glBindTexture(colourTextureTarget, 0); AN_GL_CHECK_FOR_ERROR

				// create the frame buffer
				DIRECT_GL_CALL_ glGenFramebuffers(1, &eye.frameBuffers[i]); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, eye.frameBuffers[i]); AN_GL_CHECK_FOR_ERROR
				if (useSetup.should_use_msaa())
				{
					DIRECT_GL_CALL_ glExtensions.glFramebufferTexture2DMultisampleEXT(frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colourTexture, 0, useSetup.get_msaa_samples()); AN_GL_CHECK_FOR_ERROR
				}
				else
				{
					DIRECT_GL_CALL_ glFramebufferTexture2D(frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colourTexture, 0); AN_GL_CHECK_FOR_ERROR
				}
				{
					DIRECT_GL_CALL_ GLenum renderFramebufferStatus = glCheckFramebufferStatus(frameBufferTarget); AN_GL_CHECK_FOR_ERROR
					if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
					{
						error(TXT("render frame buffer incomplete"));
						return;
					}
				}
				DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, 0); AN_GL_CHECK_FOR_ERROR

				// create and attach depth buffer (only for direct to vr rendering)
				if (_useDirectToVRRendering)
				{
					// create the depth buffer
					DIRECT_GL_CALL_ glGenRenderbuffers(1, &eye.depthBuffers[i]); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glBindRenderbuffer(GL_RENDERBUFFER, eye.depthBuffers[i]); AN_GL_CHECK_FOR_ERROR
					int depthType = ::System::DepthStencilFormat::has_stencil(useSetup.get_depth_stencil_format()) ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24; // force 24
					if (useSetup.should_use_msaa())
					{
						DIRECT_GL_CALL_ glExtensions.glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, useSetup.get_msaa_samples(), depthType, useSetup.get_size().x, useSetup.get_size().y); AN_GL_CHECK_FOR_ERROR
					}
					else
					{
						DIRECT_GL_CALL_ glRenderbufferStorage(GL_RENDERBUFFER, depthType, useSetup.get_size().x, useSetup.get_size().y); AN_GL_CHECK_FOR_ERROR
					}
					DIRECT_GL_CALL_ glBindRenderbuffer(GL_RENDERBUFFER, 0); AN_GL_CHECK_FOR_ERROR

					DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, eye.frameBuffers[i]); AN_GL_CHECK_FOR_ERROR

					// attach, each component on its own it
					DIRECT_GL_CALL_ glFramebufferRenderbuffer(frameBufferTarget, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, eye.depthBuffers[i]); AN_GL_CHECK_FOR_ERROR
					if (::System::DepthStencilFormat::has_stencil(useSetup.get_depth_stencil_format()))
					{
						DIRECT_GL_CALL_ glFramebufferRenderbuffer(frameBufferTarget, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, eye.depthBuffers[i]); AN_GL_CHECK_FOR_ERROR
					}
					DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, 0); AN_GL_CHECK_FOR_ERROR
				}

				if (_useDirectToVRRendering &&
					usingFoveatedRendering)
				{
#ifdef OWN_FOVEATED_IMPLEMENTATION
					::System::FoveatedRenderingSetup frs;
					frs.setup(renderInfo[rm].lensCentreOffset[eyeIdx], useSetup.get_size().to_vector2());
					::System::RenderTargetUtils::setup_foveated_rendering_for_texture(colourTexture, frs);
#endif
				}

				{
					DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, eye.frameBuffers[i]); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ GLenum renderFramebufferStatus = glCheckFramebufferStatus(frameBufferTarget); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, 0); AN_GL_CHECK_FOR_ERROR

					if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
					{
						error(TXT("render frame buffer incomplete"));
						return;
					}
				}

				// render targets for "direct to vr"
				if (_useDirectToVRRendering)
				{
					System::RenderTargetSetupProxy proxySetup(useSetup);
					proxySetup.colourTexture = colourTexture;
					proxySetup.frameBufferObject = eye.frameBuffers[i];
					proxySetup.depthStencilBufferObject = eye.depthBuffers[i];
					RENDER_TARGET_CREATE_INFO(TXT("oculus mobile direct to vr render target, eye %i"), i);
					eye.directToVRRenderTargets[i] = new ::System::RenderTarget(proxySetup);
				}
#endif
			}
		}
	}
}

::System::RenderTarget* OculusMobileImpl::get_direct_to_vr_render_target(int _eyeIdx)
{
	if (_eyeIdx < 2)
	{
		auto& eye = get_oculus_render_info().eye[_eyeIdx];
		auto* rt = eye.directToVRRenderTargets.is_index_valid(eye.textureSwapChainIndex)? eye.directToVRRenderTargets[eye.textureSwapChainIndex].get() : nullptr;
		if (rt)
		{
#ifdef OWN_FOVEATED_IMPLEMENTATION
			::System::FoveatedRenderingSetup frs;
			frs.setup(renderInfo[renderMode].lensCentreOffset[_eyeIdx], rt->get_size().to_vector2());
			::System::RenderTargetUtils::setup_foveated_rendering_for_texture(rt->get_data_texture_id(0), frs);
#endif
		}
		return rt;
	}
	return nullptr;
}

void OculusMobileImpl::on_render_targets_created()
{
}

void OculusMobileImpl::setup_rendering_on_init_video(::System::Video3D* _v3d)
{
	an_assert(is_ok(), TXT("openvr should be working"));
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));

	ovrTracking2 const trackingState = vrapi_GetPredictedTracking2(ovr, 0.0f);

	for_count(int, rm, InternalRenderMode::COUNT)
	{
		auto& ri = renderInfo[rm];
		for (int eyeIdx = 0; eyeIdx < 2; ++eyeIdx)
		{
			VectorInt2 idealTextureSize;
			idealTextureSize.x = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
			idealTextureSize.y = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);
			VRFovPort fp;
			ovrMatrix4f_ExtractFov(&trackingState.Eye[eyeIdx].ProjectionMatrix, &fp.left, &fp.right, &fp.up, &fp.down);
			// make values negative as ovr gives positive values only
			fp.left = -fp.left;
			fp.down = -fp.down;

			// store render info
			ri.eyeResolution[eyeIdx].x = idealTextureSize.x;
			ri.eyeResolution[eyeIdx].y = idealTextureSize.y;
			ri.fov[eyeIdx] = fp.get_fov();
			ri.fovPort[eyeIdx] = fp;
			ri.aspectRatio[eyeIdx] = aspect_ratio(ri.eyeResolution[eyeIdx]);
			ri.lensCentreOffset[eyeIdx] = fp.get_lens_centre_offset();
			// view matrix is inversed
			auto hmdToEyeTransformLeft = get_transform_from(trackingState.HeadPose.Pose).to_local(get_transform_from(ovrMatrix4f_Inverse(&trackingState.Eye[eyeIdx].ViewMatrix)));
			ri.eyeOffset[eyeIdx] = hmdToEyeTransformLeft;
		}
		viewInHeadPoseSpace = Transform::lerp(0.5f, ri.eyeOffset[0], ri.eyeOffset[1]);
		if (ri.eyeOffset[Eye::Left].get_translation().x > 0.0f)
		{
			error_stop(TXT("why eyes are not on the right side?"));
		}
	}

	for_count(int, rm, InternalRenderMode::COUNT)
	{
		auto& ri = renderInfo[rm];
		for (int eyeIdx = 0; eyeIdx < 2; ++eyeIdx)
		{
			ri.eyeOffset[eyeIdx] = viewInHeadPoseSpace.to_local(ri.eyeOffset[eyeIdx]);
		}
	}
}

Meshes::Mesh3D* OculusMobileImpl::load_mesh(Name const & _modelName) const
{
#ifdef AN_USE_VR_RENDER_MODELS
#endif
	return nullptr;
}

int OculusMobileImpl::translate_bone_index(VRHandBoneIndex::Type _index) const
{
	if (_index == VRHandBoneIndex::Wrist) return ovrHandBone_WristRoot;
	if (_index == VRHandBoneIndex::ThumbBase) return ovrHandBone_Thumb1;
	if (_index == VRHandBoneIndex::PointerBase) return ovrHandBone_Index1;
	if (_index == VRHandBoneIndex::MiddleBase) return ovrHandBone_Middle1;
	if (_index == VRHandBoneIndex::RingBase) return ovrHandBone_Ring1;
	if (_index == VRHandBoneIndex::PinkyBase) return ovrHandBone_Pinky1;
	if (_index == VRHandBoneIndex::ThumbMid) return ovrHandBone_Thumb2;
	if (_index == VRHandBoneIndex::PointerMid) return ovrHandBone_Index2;
	if (_index == VRHandBoneIndex::MiddleMid) return ovrHandBone_Middle2;
	if (_index == VRHandBoneIndex::RingMid) return ovrHandBone_Ring2;
	if (_index == VRHandBoneIndex::PinkyMid) return ovrHandBone_Pinky2;
	if (_index == VRHandBoneIndex::ThumbTip) return ovrHandBone_ThumbTip;
	if (_index == VRHandBoneIndex::PointerTip) return ovrHandBone_IndexTip;
	if (_index == VRHandBoneIndex::MiddleTip) return ovrHandBone_MiddleTip;
	if (_index == VRHandBoneIndex::RingTip) return ovrHandBone_RingTip;
	if (_index == VRHandBoneIndex::PinkyTip) return ovrHandBone_PinkyTip;
	return NONE;
}

DecideHandsResult::Type OculusMobileImpl::decide_hands(OUT_ int & _leftHand, OUT_ int & _rightHand)
{
	if (!is_ok())
	{
		// we won't ever decide
		return DecideHandsResult::CantTellAtAll;
	}

	DecideHandsResult::Type result = DecideHandsResult::Decided;
#ifdef DEBUG_DECIDE_HANDS
	output(TXT("oculus mobile : decide hands"));
#endif
	_leftHand = NONE;
	_rightHand = NONE;
	for_count(int, i, 2)
	{
		if (handControllerTrackDevices[i].controllerType != NONE)
		{
			if (handControllerTrackDevices[i].hand == Hand::Left)
			{
				_leftHand = i;
			}
			else if (handControllerTrackDevices[i].hand == Hand::Right)
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

void OculusMobileImpl::force_bounds_rendering(bool _force)
{
	todo_note(TXT("they don't disappear!"));
	/*
	if (ovr)
	{
		vrapi_RequestBoundaryVisible(ovr, _force);
	}
	*/
}

float OculusMobileImpl::update_expected_frame_time()
{
	if (!is_ok())
	{
		return 1.0f / 72.0f; // fallback
	}
	update_perf_stats();
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
		frequency = vrapi_GetSystemPropertyFloat(&java, VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE);
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

float OculusMobileImpl::calculate_ideal_expected_frame_time() const
{
	if (!is_ok())
	{
		return 1.0f / 72.0f; // fallback
	}
	float frequency = 0.0f;
	if (frequency == 0.0f)
	{
		frequency = vrapi_GetSystemPropertyFloat(&java, VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE);
	}
	if (frequency != 0.0f)
	{
		return 1.0f / frequency;
	}
	else
	{
		return 1.0f / 72.0f;
	}
}

void OculusMobileImpl::update_hands_controls_source()
{
	// we depend on being forced
	for_count(int, index, Hands::Count)
	{
		owner->access_controls().hands[index].source = VR::Input::Devices::find(modelNumber, trackingSystemName, String::empty());
	}
}

void OculusMobileImpl::draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font)
{
	an_assert(ovr != nullptr, TXT("we require ovr at this point!"));

	Vector2 at = _at;
	Vector2 triggerSize = Vector2(_width, _width * 0.1f);
	Vector2 padSize = Vector2(_width, _width);
	Vector2 separation = Vector2(_width * 0.05f, _width * 0.05f);

	Vector2 buttonSize= Vector2(_width / 8.0f, _width / 8.0f);

	int iController = handControllerTrackDevices[_hand].deviceID;
	if (iController != NONE)
	{
		ovrInputStateTrackedRemote inputState;
		inputState.Header.ControllerType = (ovrControllerType)iController;
		if (run_ovr(TXT("ovr_GetInputState"), vrapi_GetCurrentInputState(ovr, handControllerTrackDevices[_hand].deviceID, &inputState.Header)))
		{
			ARRAY_STATIC(float, axes, 32);
			axes.push_back(inputState.IndexTrigger);
			axes.push_back(inputState.GripTrigger);
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
				auto const & joystick = get_vector_2_from(inputState.Joystick);
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

void OculusMobileImpl::update_render_scaling(REF_ float & _scale)
{
	todo_note(TXT("implement!"));
	/*
	float targetScale = MainConfig::global().get_video().allowAutoAdjustForVR? _scale * perfStats.AdaptiveGpuPerformanceScale : 1.0f;
	_scale = blend_to_using_speed_based_on_time(_scale, targetScale, 0.5f, ::System::Core::get_raw_delta_time());
	_scale = clamp(_scale, 0.6f, 1.0f);
	*/
}

void OculusMobileImpl::update_perf_stats()
{
	todo_note(TXT("implement!"));
	/*
	if (perfStatsFrame != frameIndexToSubmit)
	{
		perfStatsFrame = frameIndexToSubmit;
		MEASURE_PERFORMANCE(getPerfStats);
		perfStats = {};
		ovr_GetPerfStats(ovr, &perfStats);
	}
	*/
}

void OculusMobileImpl::set_render_mode(RenderMode::Type _mode)
{
	if (_mode == RenderMode::Normal)
	{
		pendingRenderMode = InternalRenderMode::Normal;
	}
	else if (_mode == RenderMode::HiRes)
	{
		pendingRenderMode = InternalRenderMode::HiRes;
	}
}

void OculusMobileImpl::update_perf_threads()
{
	if (ovr)
	{
		if (mainThreadTid.is_set())
		{
			vrapi_SetPerfThread(ovr, VRAPI_PERF_THREAD_TYPE_MAIN, mainThreadTid.get());
		}
		if (renderThreadTid.is_set())
		{
			vrapi_SetPerfThread(ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, renderThreadTid.get());
		}
	}
}

void OculusMobileImpl::set_processing_levels_impl(Optional<float> const& _cpuLevel, Optional<float> const& _gpuLevel, bool _temporarily)
{
	int useCPULevel = cpuLevel;
	int useGPULevel = gpuLevel;
	if (_cpuLevel.is_set())
	{
		useCPULevel = TypeConversions::Normal::f_i_closest(_cpuLevel.get() * 4.0f);
	}
	if (_gpuLevel.is_set())
	{
		useGPULevel = TypeConversions::Normal::f_i_closest(_gpuLevel.get() * 4.0f);
	}
	if (!_temporarily)
	{
		cpuLevel = useCPULevel;
		gpuLevel = useGPULevel;
	}
	if (ovr)
	{
		vrapi_SetClockLevels(ovr, useCPULevel, useGPULevel);
	}
}

void OculusMobileImpl::I_am_perf_thread_main_impl()
{
	int tid = gettid();
	if (!mainThreadTid.is_set() || mainThreadTid.get() != tid)
	{
		mainThreadTid = tid;
		if (ovr)
		{
			vrapi_SetPerfThread(ovr, VRAPI_PERF_THREAD_TYPE_MAIN, tid);
			output(TXT("main perf thread (thread priority) is %i"), tid);
		}
	}
}

void OculusMobileImpl::I_am_perf_thread_render_impl()
{
	int tid = gettid();
	if (!renderThreadTid.is_set() || renderThreadTid.get() != tid)
	{
		renderThreadTid = tid;
		if (ovr)
		{
			vrapi_SetPerfThread(ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, tid);
			output(TXT("render perf thread (thread priority) is %i"), tid);
		}
	}
}

#endif
