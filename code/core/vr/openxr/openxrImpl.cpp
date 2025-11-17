#include "openxrImpl.h"

#ifdef AN_WITH_OPENXR
#include "..\vrFovPort.h"

#include "..\..\mainConfig.h"

#include "..\..\concurrency\scopedSpinLock.h"
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

#ifdef AN_ANDROID
#include <unistd.h>
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef BUILD_NON_RELEASE
	#ifndef AN_DEVELOPMENT
		//#define OUTPUT_ALL_LONG_WAITS
		//#define OUTPUT_ALL_LONG_WAITS_ALL
	#endif
#endif

#define INSPECT_VIVE_COSMOS

// we use semaphore and semaphore hogs onto CPU
//#define USE_EXTRA_WAIT_FOR_BEGIN_RENDER
//#define USE_EXTRA_WAIT_FOR_GAME_SYNC

//#define AN_OPEN_XR_EXTENDED_DEBUG

//#define OUTPUT_SWAPCHAIN_FORMATS
#ifdef INSPECT_VIVE_COSMOS
	#define OUTPUT_SWAPCHAIN_FORMATS
#endif

//#define ELABORATE_DEBUG_LOGGING

//#define DEBUG_READING_POSES

//#define AN_LOG_FRAME_CALLS

// this is for direct rendering - doesn't work with openxr, works with own foveated implementation
#define OWN_FOVEATED_IMPLEMENTATION
//#define OPENXR_FOVEATED_IMPLEMENTATION_FB
//#define OPENXR_FOVEATED_IMPLEMENTATION_HTC

// only one should be active
#ifndef OWN_FOVEATED_IMPLEMENTATION
	#ifdef AN_OCULUS_ONLY
		#ifndef OPENXR_FOVEATED_IMPLEMENTATION_FB
			#define OPENXR_FOVEATED_IMPLEMENTATION_FB
		#endif
	#endif
	#ifdef AN_VIVE_ONLY
		#ifndef OPENXR_FOVEATED_IMPLEMENTATION_HTC
			#define OPENXR_FOVEATED_IMPLEMENTATION_HTC
		#endif
	#endif
#endif
#ifdef OWN_FOVEATED_IMPLEMENTATION
	#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
		#undef OPENXR_FOVEATED_IMPLEMENTATION_FB
	#endif
	#ifdef OPENXR_FOVEATED_IMPLEMENTATION_HTC
		#undef OPENXR_FOVEATED_IMPLEMENTATION_HTC
	#endif
#endif

#define SETUP_FOVEATION_RIGHT_AFTER_CREATED_SWAPCHAIN
// avoid every frame!
//#define UPDATE_FOVEATION_EVERY_FRAME
//#define UPDATE_FOVEATION_EVERY_FRAME_CREATE_PROFILE

#ifdef AN_PICO
	// pico does not require hacky workarounds
#else
	#define USE_HACKY_WORKAROUND_FOR_RECENTER__RECREATE_SESSION_FOR_RECENTER
	//#define USE_HACKY_WORKAROUND_FOR_RECENTER__OFFSET_BY_DIFF
#endif

//

// extensions (missing from openxr.h)
#define XR_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME "XR_EXT_hp_mixed_reality_controller"
#define XR_PICO_CONTROLLER_INTERACTION_EXTENSION_NAME "XR_PICO_controller_interaction"

//

#ifdef AN_VIVE_ONLY
	#define DEFAULT_DISPLAY_REFRESH_RATE 90.0f
#else
	#define DEFAULT_DISPLAY_REFRESH_RATE 72.0f
#endif

//

using namespace VR;

//

#ifdef AN_CLANG
#define tstrcpy strcpy
#else
#define tstrcpy strcpy_s
#endif

static Vector3 get_vector_from(XrVector3f const& _v)
{
	// openxr: x right y up -z forward
	// our: x right y forward z up
	return Vector3(_v.x, -_v.z, _v.y);
}

static Vector2 get_vector2_from(XrVector2f const& _v)
{
	return Vector2(_v.x, _v.y);
}

static Quat get_quat_from(XrQuaternionf const& _quat)
{
	return Quat(_quat.x, -_quat.z, _quat.y, _quat.w);
}

static Transform get_transform_from(XrPosef const& _pose)
{
	return Transform(get_vector_from(_pose.position), get_quat_from(_pose.orientation));
}

static void sanitise_xr_bounds_size(REF_ XrExtent2Df & xrBounds)
{
	// some systems return bounds size in milimeters
	if (xrBounds.width > MAX_VR_SIZE &&
		xrBounds.height > MAX_VR_SIZE)
	{
		xrBounds.width /= 1000.0f;
		xrBounds.height /= 1000.0f;
	}
}

//

void OpenXRImpl::get_ready_to_exit_impl()
{
	// clear and submit empty?
}

String OpenXRImpl::get_xr_result_to_string(XrResult _result) const
{
	char resultString8[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(xrInstance, _result, resultString8);

	String resultString = String::from_char8(resultString8);

	return resultString;
}

bool OpenXRImpl::check_openxr(XrResult _result, tchar const* _format = TXT("[none provided]"), ...) const
{
	//if (XR_SUCCEEDED(_result))
	if (_result == XR_SUCCESS)
	{
		return true;
	}

	String resultString = get_xr_result_to_string(_result);

#ifndef AN_CLANG
	int formatsize = (int)(tstrlen(_format) + 1);
	int memsize = (int)(formatsize * sizeof(tchar));
	allocate_stack_var(tchar, format, formatsize);
	memory_copy(format, _format, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _format;
#endif

	tchar message[XR_MAX_RESULT_STRING_SIZE + 1024];

	va_list list;
	va_start(list, _format);
	tvsprintf(message, XR_MAX_RESULT_STRING_SIZE + 1023, format, list);
	va_end(list);

	if (XR_SUCCEEDED(_result))
	{
		if (_result != XR_SESSION_NOT_FOCUSED) // as this may happen when we take the helmet off
		{
			warn(TXT("[system-openxr] succeeded but sort of not (%i:%S): %S"), _result, resultString.to_char(), message);
		}

		return true;
	}
	else
	{
		error(TXT("[system-openxr] error (%i:%S): %S"), _result, resultString.to_char(), message);

		return false;
	}
}

bool OpenXRImpl::check_openxr_warn(XrResult _result, tchar const* _format = TXT("[none provided]"), ...) const
{
	//if (XR_SUCCEEDED(_result))
	if (_result == XR_SUCCESS)
	{
		return true;
	}

	String resultString = get_xr_result_to_string(_result);

#ifndef AN_CLANG
	int formatsize = (int)(tstrlen(_format) + 1);
	int memsize = (int)(formatsize * sizeof(tchar));
	allocate_stack_var(tchar, format, formatsize);
	memory_copy(format, _format, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _format;
#endif

	tchar message[XR_MAX_RESULT_STRING_SIZE + 1024];

	va_list list;
	va_start(list, _format);
	tvsprintf(message, XR_MAX_RESULT_STRING_SIZE + 1023, format, list);
	va_end(list);

	if (XR_SUCCEEDED(_result))
	{
		if (_result != XR_SESSION_NOT_FOCUSED) // as this may happen when we take the helmet off
		{
			warn(TXT("[system-openxr] succeeded but sort of not (%i:%S): %S"), _result, resultString.to_char(), message);
		}

		return true;
	}
	else
	{
		warn(TXT("[system-openxr] problem (%i:%S): %S"), _result, resultString.to_char(), message);

		return false;
	}
}

String OpenXRImpl::get_path_string(XrPath _path) const
{
	uint32_t count;
	check_openxr(xrPathToString(xrInstance, _path, 0, &count, nullptr));
	Array<char> string8;
	string8.set_size(count + 1);
	check_openxr(xrPathToString(xrInstance, _path, count, &count, string8.get_data()));
	string8[count] = 0;
	return String::from_char8(string8.get_data());
}

bool OpenXRImpl::load_ext_function(char const* _functionName, void* _function, bool _optional)
{
	XrResult result = xrGetInstanceProcAddr(xrInstance, _functionName, (PFN_xrVoidFunction*)_function);
	if (!check_openxr(result, TXT("get ext function \"%S\""), String::from_char8(_functionName).to_char()))
	{
		if (! _optional)
		{
			isOk = false;
		}
		return false;
	}

	return true;
}

//

bool OpenXRImpl::is_available()
{
	// we assume openxr is always available?
	return true;
}

OpenXRImpl::OpenXRImpl(OpenXR* _owner, bool _firstChoice)
: base(_owner)
, firstChoice(_firstChoice)
{
#ifdef AN_OPENXR_WITH_HAND_TRACKING
	for_count(int, i, Hand::MAX)
	{
		xrHandTrackers[i] = XR_NULL_HANDLE;
	}
#endif
}

void OpenXRImpl::init_impl()
{
	System::Core::access_system_tags().set_tag(Name(TXT("openxr")));
	Splash::Logos::add(TXT("openxr"), SPLASH_LOGO_IMPORTANT);

	output(TXT("[system-openxr] initialising openxr"));

#ifdef AN_ANDROID
	{
		PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
		xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
		if (xrInitializeLoaderKHR)
		{
			output(TXT("[system-openxr] initialising loader (android)"));
			XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid;
			memory_zero(loaderInitializeInfoAndroid);
			loaderInitializeInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
			loaderInitializeInfoAndroid.next = nullptr;
			loaderInitializeInfoAndroid.applicationVM = ::System::Android::App::get().activityVM;
			loaderInitializeInfoAndroid.applicationContext = ::System::Android::App::get().activityObject;
			xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&loaderInitializeInfoAndroid);
		}
	}
#endif

	{
		auto& sysPrecName = ::System::Core::get_system_precise_name();
		systemBasedRefreshRate = DEFAULT_DISPLAY_REFRESH_RATE;
		if (sysPrecName == TXT("Quest Pro") ||
			sysPrecName == TXT("VIVE Focus 3"))
		{
			systemBasedRefreshRate = 90.0f;
		}
		output(TXT("[system-openxr] assume system based display referesh rate %.0fHz for \"%S\""), systemBasedRefreshRate.get(), sysPrecName.to_char());
	}

	// check extension to make sure they're supported
	Array<const char*> extReq;
#ifdef AN_WINDOWS
	extReq.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
#endif
#ifdef AN_ANDROID
	extReq.push_back(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
#endif

	Array<const char*> extOpt;
	extOpt.push_back(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
#ifdef AN_WINDOWS
	extOpt.push_back(XR_OCULUS_AUDIO_DEVICE_GUID_EXTENSION_NAME);
#endif
#ifdef AN_ANDROID
	extOpt.push_back(XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME);
#endif
	extOpt.push_back(XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME);
#ifdef XR_USE_META_METRICS
	extOpt.push_back(XR_META_PERFORMANCE_METRICS_EXTENSION_NAME);
#endif
#ifdef BUILD_PREVIEW
	extOpt.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	//extOpt.push_back(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);	
	extOpt.push_back(XR_FB_COLOR_SPACE_EXTENSION_NAME);
	extOpt.push_back(XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME);
#ifdef XR_USE_GET_BOUND_2D
	extOpt.push_back(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME);
	extOpt.push_back(XR_FB_SCENE_EXTENSION_NAME);
#endif
#ifdef AN_GLES
	extOpt.push_back(XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME);
#endif
	extOpt.push_back(XR_FB_FOVEATION_EXTENSION_NAME);
	extOpt.push_back(XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME);
	extOpt.push_back(XR_HTC_FOVEATION_EXTENSION_NAME);

#ifdef AN_OPENXR_WITH_HAND_TRACKING
	extOpt.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
	extOpt.push_back(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
	extOpt.push_back(XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME);
	extOpt.push_back(XR_HTC_HAND_INTERACTION_EXTENSION_NAME);
#endif

	// input extensions
	extOpt.push_back(XR_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME);
	extOpt.push_back(XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME);
	extOpt.push_back(XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME);
	extOpt.push_back(XR_PICO_CONTROLLER_INTERACTION_EXTENSION_NAME);

	output(TXT("[system-openxr] get extensions"));
	uint32_t extCount = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
	Array<XrExtensionProperties> extProps;
	extProps.set_size(extCount);
	for_every(ep, extProps)
	{
		ep->type = XR_TYPE_EXTENSION_PROPERTIES;
	}
	xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, extProps.begin());

	// we have all extensions available listed, check which one we asked and store them
	Array<const char*> & extUse = extensionsInUse;
	Array<const char*> extUseReq;
	extUse.clear();
	for_every(ep, extProps)
	{
		String extName;
		extName = String::from_char8(ep->extensionName);
		output(TXT("- %S"), extName.to_char());

		for_every_ptr(ea, extReq)
		{
			if (strcmp(ea, ep->extensionName) == 0)
			{
				extUse.push_back(ea);
				extUseReq.push_back(ea);
				break;
			}
		}
		for_every_ptr(ea, extOpt)
		{
			if (strcmp(ea, ep->extensionName) == 0)
			{
				extUse.push_back(ea);
				break;
			}
		}
	}

	if (extReq.get_size() != extUseReq.get_size())
	{
		error(TXT("[system-openxr] could not initialise all crucial extensions! (%i v %i)"), extReq.get_size(), extUseReq.get_size());
		for_every_ptr(er, extReq)
		{
			bool exists = false;
			for_every_ptr(eur, extUseReq)
			{
				if (strcmp(er, eur) == 0)
				{
					exists = true;
					break;
				}
			}
			if (!exists)
			{
				String extName;
				extName = String::from_char8(er);
				error(TXT("[system-openxr] - %S"), extName.to_char());
			}
		}
		return;
	}

	// initialise xr instance
	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount = extUse.get_size();
	createInfo.enabledExtensionNames = extUse.get_data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	{
		String appName(::System::Core::get_app_name());
		auto appName8 = appName.to_char8_array();
		tstrcpy(createInfo.applicationInfo.applicationName, appName8.get_data());
	}

#ifdef AN_ANDROID
	XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid = { XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR };
	{
		instanceCreateInfoAndroid.applicationVM = ::System::Android::App::get().activityVM;
		instanceCreateInfoAndroid.applicationActivity = ::System::Android::App::get().activityObject;
		createInfo.next = &instanceCreateInfoAndroid;
	}
#endif

	xrCreateInstance(&createInfo, &xrInstance);

	if (xrInstance == XR_NULL_HANDLE)
	{
		error(TXT("[system-openxr] could not initialise openxr"));
	}

	output(TXT("[system-openxr] extensions used"));
	for_every(ext, extUse)
	{
		String extName;
		extName = String::from_char8(*ext);
		output(TXT("- %S"), extName.to_char());
	}

#ifdef AN_GL
	load_ext_function("xrGetOpenGLGraphicsRequirementsKHR", &ext_xrGetOpenGLGraphicsRequirementsKHR);
#endif
#ifdef AN_GLES
	load_ext_function("xrGetOpenGLESGraphicsRequirementsKHR", &ext_xrGetOpenGLESGraphicsRequirementsKHR);
#endif

	if (extUse.does_contain(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME))
	{
		load_ext_function("xrGetDisplayRefreshRateFB", &ext_xrGetDisplayRefreshRateFB);
	}
#ifdef AN_WINDOWS
	if (extUse.does_contain(XR_OCULUS_AUDIO_DEVICE_GUID_EXTENSION_NAME))
	{
		load_ext_function("xrGetAudioOutputDeviceGuidOculus", &ext_xrGetAudioOutputDeviceGuidOculus);
	}
#endif
#ifdef AN_ANDROID
	if (extUse.does_contain(XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME))
	{
		load_ext_function("xrSetAndroidApplicationThreadKHR", &ext_xrSetAndroidApplicationThreadKHR);
	}
#endif
	if (extUse.does_contain(XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME))
	{
		load_ext_function("xrPerfSettingsSetPerformanceLevelEXT", &ext_xrPerfSettingsSetPerformanceLevelEXT);
	}
#ifdef XR_USE_META_METRICS
	if (extUse.does_contain(XR_META_PERFORMANCE_METRICS_EXTENSION_NAME))
	{
		load_ext_function("xrSetPerformanceMetricsStateMETA", &ext_xrSetPerformanceMetricsStateMETA);
		load_ext_function("xrEnumeratePerformanceMetricsCounterPathsMETA", &ext_xrEnumeratePerformanceMetricsCounterPathsMETA);
		load_ext_function("xrQueryPerformanceMetricsCounterMETA", &ext_xrQueryPerformanceMetricsCounterMETA);
	}
#endif

#ifdef OWN_FOVEATED_IMPLEMENTATION
	xrFoveationAvailable = false;
#endif
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
	xrFoveationAvailable = extUse.does_contain(XR_FB_FOVEATION_EXTENSION_NAME) && extUse.does_contain(XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME);
	if (xrFoveationAvailable)
	{
		output(TXT("[system-openxr] use foveation (fb)"));

		load_ext_function("xrCreateFoveationProfileFB", &ext_xrCreateFoveationProfileFB);
		load_ext_function("xrDestroyFoveationProfileFB", &ext_xrDestroyFoveationProfileFB);
		load_ext_function("xrUpdateSwapchainFB", &ext_xrUpdateSwapchainFB);

		if (!ext_xrCreateFoveationProfileFB ||
			!ext_xrDestroyFoveationProfileFB ||
			!ext_xrUpdateSwapchainFB)
		{
			output(TXT("[system-openxr] disabled foveation due to lack of functions"));
			xrFoveationAvailable = false;
		}
	}
#endif
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_HTC
	xrFoveationAvailable = extUse.does_contain(XR_HTC_FOVEATION_EXTENSION_NAME);
	if (xrFoveationAvailable)
	{
		output(TXT("[system-openxr] use foveation (htc)"));

		load_ext_function("xrApplyFoveationHTC", &ext_xrApplyFoveationHTC);

		if (!ext_xrApplyFoveationHTC)
		{
			output(TXT("[system-openxr] disabled foveation due to lack of functions"));
			xrFoveationAvailable = false;
		}
	}
#endif

	handTrackingAvailable = false; // set later
	handTrackingSupported = false;
	handTrackingAimSupported = false;
	handTrackingMeshSupported = false;
#ifdef AN_OPENXR_WITH_HAND_TRACKING
	if (extUse.does_contain(XR_EXT_HAND_TRACKING_EXTENSION_NAME))
	{
		handTrackingSupported = true;
		handTrackingAimSupported = extUse.does_contain(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
		handTrackingMeshSupported = extUse.does_contain(XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME);

		load_ext_function("xrCreateHandTrackerEXT", &ext_xrCreateHandTrackerEXT);
		load_ext_function("xrDestroyHandTrackerEXT", &ext_xrDestroyHandTrackerEXT);
		load_ext_function("xrLocateHandJointsEXT", &ext_xrLocateHandJointsEXT);
		if (handTrackingMeshSupported)
		{
			load_ext_function("xrGetHandMeshFB", &ext_xrGetHandMeshFB);
		}
	}
#endif

	if (extUse.does_contain(XR_FB_COLOR_SPACE_EXTENSION_NAME))
	{
		load_ext_function("xrSetColorSpaceFB", &ext_xrSetColorSpaceFB);
	}

#ifdef XR_USE_GET_BOUND_2D
	if (extUse.does_contain(XR_FB_SCENE_EXTENSION_NAME))
	{
		load_ext_function("xrGetSpaceBoundary2DFB", &ext_xrGetSpaceBoundary2DFB);
	}
	if (extUse.does_contain(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME))
	{
		load_ext_function("xrSetSpaceComponentStatusFB", &ext_xrSetSpaceComponentStatusFB);
		load_ext_function("xrGetSpaceComponentStatusFB", &ext_xrGetSpaceComponentStatusFB);
		load_ext_function("xrCreateSpatialAnchorFB", &ext_xrCreateSpatialAnchorFB);
	}
#endif

#ifdef BUILD_PREVIEW
	// setup debug
	if (extUse.does_contain(XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		// get extension module methods we're to use
		load_ext_function("xrCreateDebugUtilsMessengerEXT", &ext_xrCreateDebugUtilsMessengerEXT, true);
		load_ext_function("xrDestroyDebugUtilsMessengerEXT", &ext_xrDestroyDebugUtilsMessengerEXT, true);

		if (ext_xrCreateDebugUtilsMessengerEXT &&
			ext_xrDestroyDebugUtilsMessengerEXT)
		{
			XrDebugUtilsMessengerCreateInfoEXT debug_info = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			debug_info.messageTypes =
				XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
				XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
			debug_info.messageSeverities =
				XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debug_info.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT _severity, XrDebugUtilsMessageTypeFlagsEXT _types, const XrDebugUtilsMessengerCallbackDataEXT* _msg, void* _userData)
			{
				String fnName;
				String msg;
				fnName = String::from_char8(_msg->functionName);
				msg = String::from_char8(_msg->message);

				output(TXT("[system-openxr] [debug] %S: %S"), fnName.to_char(), msg.to_char());

				// XR_TRUE means we should fail
				return (XrBool32)XR_FALSE;
			};

			// Start up the debug utils!
			if (ext_xrCreateDebugUtilsMessengerEXT)
			{
				ext_xrCreateDebugUtilsMessengerEXT(xrInstance, &debug_info, &xrDebug);
			}
		}
	}
#endif

#ifdef AN_WINDOWS
	if (ext_xrGetAudioOutputDeviceGuidOculus)
	{
		tchar guidStrChars[XR_MAX_AUDIO_DEVICE_STR_SIZE_OCULUS];
		if (check_openxr(ext_xrGetAudioOutputDeviceGuidOculus(xrInstance, guidStrChars), TXT("get audio guid")))
		{
			String guidStr;
			guidStr = guidStrChars;
			int openBrackeAt = guidStr.find_last_of('{');
			if (openBrackeAt != NONE)
			{
				guidStr = guidStr.get_sub(openBrackeAt);
			}

			GUID guid;
			HRESULT hr = IIDFromString(guidStr.to_char(), &guid);
			if (hr == S_OK)
			{
				::System::Sound::set_preferred_audio_device(guid);
			}
		}
	}
#endif

#ifdef XR_USE_META_METRICS
	if (ext_xrQueryPerformanceMetricsCounterMETA)
	{
		uint32_t count;
		check_openxr(ext_xrEnumeratePerformanceMetricsCounterPathsMETA(xrInstance, 0, &count, nullptr));
		performanceMetricsCounterPathsMETA.set_size(count);
		check_openxr(ext_xrEnumeratePerformanceMetricsCounterPathsMETA(xrInstance, count, &count, performanceMetricsCounterPathsMETA.get_data()));
		output(TXT("available performance metrics:"));
		for_every(path, performanceMetricsCounterPathsMETA)
		{
			String pathString = get_path_string(*path);
			output(TXT(" - %S"), pathString.to_char());
		}
	}
#endif

	isOk = true;
}

void OpenXRImpl::shutdown_impl()
{
	if (xrInstance != XR_NULL_HANDLE)
	{
		xrDestroyInstance(xrInstance);
		xrInstance = XR_NULL_HANDLE;
	}
}

void OpenXRImpl::create_device_impl()
{
	// get system id
	{
		XrSystemGetInfo systemGetInfo;
		memory_zero(systemGetInfo);
		systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
		systemGetInfo.formFactor = formFactor;
		systemGetInfo.next = nullptr;

		XrResult result = xrGetSystem(xrInstance, &systemGetInfo, &xrSystemId);
		if (!check_openxr(result, TXT("get system")))
		{
			isOk = false;
			return;
		}
	}

	handTrackingAvailable = false;
#ifdef AN_OPENXR_WITH_HAND_TRACKING
	{
		XrSystemProperties systemProps;
		memory_zero(systemProps);
		systemProps.type = XR_TYPE_SYSTEM_PROPERTIES;
		systemProps.next = nullptr;

		XrSystemHandTrackingPropertiesEXT handTrackingProps;
		memory_zero(handTrackingProps);
		handTrackingProps.type = XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT;
		systemProps.next = &handTrackingProps;

		XrResult result = xrGetSystemProperties(xrInstance, xrSystemId, &systemProps);
		if (check_openxr(result, TXT("get system properties")))
		{
			handTrackingAvailable = handTrackingProps.supportsHandTracking;
		}
	}
#endif

	// get system name and output system props
	{
		XrSystemProperties systemProps;
		memory_zero(systemProps);
		systemProps.type = XR_TYPE_SYSTEM_PROPERTIES;
		systemProps.next = nullptr;

		XrSystemColorSpacePropertiesFB colorSpacePropertiesFB;
		if (ext_xrSetColorSpaceFB)
		{
			memory_zero(colorSpacePropertiesFB);
			colorSpacePropertiesFB.type = XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB;
			systemProps.next = &colorSpacePropertiesFB;
		}

		XrResult result = xrGetSystemProperties(xrInstance, xrSystemId, &systemProps);
		if (!check_openxr(result, TXT("get system properties")))
		{
			isOk = false;
			return;
		}

		system.name = String::from_char8(systemProps.systemName);
		system.isAndroid = false;
#ifdef AN_ANDROID
		system.isAndroid = true;
#endif
		system.isOculus = system.name.does_contain_icase(TXT("oculus"));
		system.isQuest = system.name.does_contain_icase(TXT("quest"));
		system.isVive = system.name.does_contain_icase(TXT("vive")) || system.name.does_contain_icase(TXT("wave:sue"));
		system.isPico = system.name.does_contain_icase(TXT("pico"));
		
#ifdef AN_WINDOWS
		if (firstChoice &&
			system.name == TXT("Vive OpenXR: Vive SRanipal"))
		{
			// this is done for Vive Cosmos as it is not possible to setup frame buffer with textures provided by OpenXR
			error(TXT("Vive OpenXR is not allowed to be used"));
			isOk = false;
			return;
		}
#endif

		output(TXT("[system-openxr] system \"%S\", vendor ID %d"), system.name.to_char(), systemProps.vendorId);
		output(TXT("  max layers           : %d"), systemProps.graphicsProperties.maxLayerCount);
		output(TXT("  max swapchain height : %d"), systemProps.graphicsProperties.maxSwapchainImageHeight);
		output(TXT("  max swapchain width  : %d"), systemProps.graphicsProperties.maxSwapchainImageWidth);
		output(TXT("  orientation tracking : %S"), systemProps.trackingProperties.orientationTracking? TXT("yes") : TXT("no"));
		output(TXT("  position tracking    : %S"), systemProps.trackingProperties.positionTracking? TXT("yes") : TXT("no"));
		if (ext_xrSetColorSpaceFB)
		{
			output(TXT("  color space          : %i"), colorSpacePropertiesFB.colorSpace);
		}
		if (system.isAndroid)	output(TXT("  system               : android"));
		if (system.isOculus)	output(TXT("  system               : oculus"));
		if (system.isQuest)		output(TXT("  system               : quest"));
		if (system.isVive)		output(TXT("  system               : vive"));
		if (system.isPico)		output(TXT("  system               : pico"));
	}

	// get views (config for views)
	{
		uint32_t viewCount = 0;
		{
			XrResult result = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, viewType, 0, &viewCount, nullptr);
			if (!check_openxr(result, TXT("get view configuration view count")))
			{
				isOk = false;
				return;
			}
		}

		xrConfigViews.set_size(viewCount);
		for_every(xcv, xrConfigViews)
		{
			memory_zero(*xcv);
			xcv->type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
			xcv->next = nullptr;
		}

		{
			XrResult result = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, viewType, viewCount, &viewCount, xrConfigViews.begin());

			if (!check_openxr(result, TXT("enumerate view configuration views")))
			{
				isOk = false;
				return;
			}

			output(TXT("[system-openxr] views:"));
			for_every(xcv, xrConfigViews)
			{
				output(TXT("  view %d"), for_everys_index(xcv));
				output(TXT("    recommended        : %d x %d, swapchain samples %d"), xcv->recommendedImageRectWidth, xcv->recommendedImageRectHeight, xcv->recommendedSwapchainSampleCount);
				output(TXT("    max                : %d x %d, swapchain samples %d"), xcv->maxImageRectWidth, xcv->maxImageRectHeight, xcv->maxSwapchainSampleCount);
			}

			if (!xrConfigViews.is_empty())
			{
				preferredFullScreenResolution.x = xrConfigViews.get_first().recommendedImageRectWidth;
				preferredFullScreenResolution.y = xrConfigViews.get_first().recommendedImageRectHeight;
			}

		}
	}

	// create xr views as they might be used at any time now
	{
		xrViews.set_size(xrConfigViews.get_size());
		for_every(xv, xrViews)
		{
			memory_zero(*xv);
			xv->type = XR_TYPE_VIEW;
			xv->next = nullptr;
		}
	}

	// openxr requires checking graphics requirements before a session is created
#ifdef AN_GL
	{
		XrGraphicsRequirementsOpenGLKHR openglReqs;
		memory_zero(openglReqs);
		openglReqs.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
		openglReqs.next = nullptr;

		// this function pointer was loaded with xrGetInstanceProcAddr
		XrResult result = ext_xrGetOpenGLGraphicsRequirementsKHR(xrInstance, xrSystemId, &openglReqs);
		if (!check_openxr(result, TXT("get OpenGL graphics requirements")))
		{
			isOk = false;
			return;
		}
	}
#endif
#ifdef AN_GLES
	{
		XrGraphicsRequirementsOpenGLESKHR openglesReqs;
		memory_zero(openglesReqs);
		openglesReqs.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
		openglesReqs.next = nullptr;

		// this function pointer was loaded with xrGetInstanceProcAddr
		XrResult result = ext_xrGetOpenGLESGraphicsRequirementsKHR(xrInstance, xrSystemId, &openglesReqs);
		if (!check_openxr(result, TXT("get OpenGLES graphics requirements")))
		{
			isOk = false;
			return;
		}
	}
#endif

	// check if wireless
	{
		wireless = (system.isOculus && system.isQuest) || system.isVive || system.isPico;
#ifdef AN_ANDROID
		mayHaveInvalidBoundary = false;
#else
		todo_hack(TXT("for time being we have no way to read a proper boundary"));
		mayHaveInvalidBoundary = wireless;
#endif
	}
}

void OpenXRImpl::enter_vr_mode_impl()
{
	xrViewPoseSpace = XR_NULL_HANDLE;
	for_count(int, i, Hand::MAX)
	{
		xrHandAimPoseSpaces[i] = XR_NULL_HANDLE;
		xrHandGripPoseSpaces[i] = XR_NULL_HANDLE;
	}

	memory_zero(graphicsBindingGL);
#ifdef AN_GL
#ifdef AN_WINDOWS
	graphicsBindingGL.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
#else
#error implement
#endif
#endif
#ifdef AN_GLES
#ifdef AN_ANDROID
	graphicsBindingGL.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
#else
#error implement
#endif
#endif
	graphicsBindingGL.next = nullptr;

#ifdef AN_WINDOWS
	HWND hwnd;
	HDC hdc;
	::System::Video3D::get()->get_hwnd_and_hdc(hwnd, hdc);
	graphicsBindingGL.hGLRC = wglGetCurrentContext();
	graphicsBindingGL.hDC = hdc;
#endif
#ifdef AN_ANDROID
	auto* eglContext = ::System::Video3D::get()->get_gl_context();
	graphicsBindingGL.display = eglContext->display;
	graphicsBindingGL.config = eglContext->config;
	graphicsBindingGL.context = eglContext->context;
	output(TXT("[system-openxr] create session"));
	//output(TXT("  display: %p"), eglContext->display);
	//output(TXT("  config : %p"), eglContext->config);
	//output(TXT("  context: %p"), eglContext->context);
#endif

	{
		XrSessionCreateInfo sessionCreateInfo;
		memory_zero(sessionCreateInfo);
		sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
		sessionCreateInfo.next = &graphicsBindingGL;
		sessionCreateInfo.systemId = xrSystemId;

		XrResult result = xrCreateSession(xrInstance, &sessionCreateInfo, &xrSession);
		if (!check_openxr(result, TXT("create session")))
		{
			isOk = false;
			return;
		}
	}

	{
		uint32_t availableSpacesCount = 0;
		check_openxr(xrEnumerateReferenceSpaces(xrSession, 0, &availableSpacesCount, NULL), TXT("enumerate reference spaces"));

		Array<XrReferenceSpaceType> availableSpaces;
		availableSpaces.set_size(availableSpacesCount);

		check_openxr(xrEnumerateReferenceSpaces(xrSession, availableSpacesCount, &availableSpacesCount, availableSpaces.begin()), TXT("enumerate reference spaces [2]"));

		bool spaceSupported = false;
		for_every(availableSpace, availableSpaces)
		{
			if (*availableSpace == referenceSpaceType)
			{
				spaceSupported = true;
			}
		}

		if (!spaceSupported)
		{
			error(TXT("[system-openxr] requested reference space not supported"));
		}
	}

	{
		XrReferenceSpaceCreateInfo playSpaceCreateInfo;
		memory_zero(playSpaceCreateInfo);
		playSpaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		playSpaceCreateInfo.next = nullptr;
		playSpaceCreateInfo.referenceSpaceType = referenceSpaceType;
		playSpaceCreateInfo.poseInReferenceSpace = xrIdentityPose;

		XrResult result = xrCreateReferenceSpace(xrSession, &playSpaceCreateInfo, &xrPlaySpace);
		if (!check_openxr(result, TXT("create play space")))
		{
			isOk = false;
			return;
		}
	}

	{
		XrReferenceSpaceCreateInfo viewSpaceCreateInfo;
		memory_zero(viewSpaceCreateInfo);
		viewSpaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		viewSpaceCreateInfo.next = nullptr;
		viewSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		viewSpaceCreateInfo.poseInReferenceSpace = xrIdentityPose;

		XrResult result = xrCreateReferenceSpace(xrSession, &viewSpaceCreateInfo, &xrViewPoseSpace);
		if (!check_openxr(result, TXT("create view space")))
		{
			isOk = false;
			return;
		}
	}

#ifdef XR_USE_GET_BOUND_2D
	// nothing here works
	if (ext_xrCreateSpatialAnchorFB)
	{
		XrSpatialAnchorCreateInfoFB createInfo;
		memory_zero(createInfo);
		createInfo.type = XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_FB;
		createInfo.next = nullptr;
		createInfo.space = xrPlaySpace;
		createInfo.poseInSpace = xrIdentityPose;
		XrResult result = ext_xrCreateSpatialAnchorFB(xrSession, &createInfo, &xrSpatialEntityRequestId);
		if (!check_openxr(result, TXT("get space component status")))
		{
			warn(TXT("space component set invalid: %S"), get_xr_result_to_string(result).to_char());
		}
	}

	if (ext_xrGetSpaceComponentStatusFB)
	{
		XrSpaceComponentStatusFB status;
		memory_zero(status);
		status.type = XR_TYPE_SPACE_COMPONENT_STATUS_FB;
		status.next = nullptr;

		XrResult result = ext_xrGetSpaceComponentStatusFB(xrPlaySpace, XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB, &status);
		if (!check_openxr(result, TXT("get space component status")))
		{
			warn(TXT("space component set invalid: %S"), get_xr_result_to_string(result).to_char());
		}
	}
	if (ext_xrGetSpaceComponentStatusFB)
	{
		XrSpaceComponentStatusFB status;
		memory_zero(status);
		status.type = XR_TYPE_SPACE_COMPONENT_STATUS_FB;
		status.next = nullptr;

		XrResult result = ext_xrGetSpaceComponentStatusFB(xrViewPoseSpace, XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB, &status);
		if (!check_openxr(result, TXT("get space component status")))
		{
			warn(TXT("space component set invalid: %S"), get_xr_result_to_string(result).to_char());
		}
	}

	if (ext_xrSetSpaceComponentStatusFB)
	{
		XrSpaceComponentStatusSetInfoFB spaceComponentStatusSetInfo;
		memory_zero(spaceComponentStatusSetInfo);
		spaceComponentStatusSetInfo.type = XR_TYPE_SPACE_COMPONENT_STATUS_SET_INFO_FB;
		spaceComponentStatusSetInfo.next = nullptr;
		spaceComponentStatusSetInfo.componentType = XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB;
		spaceComponentStatusSetInfo.enabled = true;

		XrResult result = ext_xrSetSpaceComponentStatusFB(xrPlaySpace, &spaceComponentStatusSetInfo, &xrSpatialEntityRequestId);
		if (!check_openxr(result, TXT("set space component")))
		{
			warn(TXT("space component set invalid: %S"), get_xr_result_to_string(result).to_char());
		}
	}
#endif

	// create paths
	{
		check_openxr(xrStringToPath(xrInstance, "/user/hand/left", &xrHandPaths[Hand::Left]), TXT("create path"));
		check_openxr(xrStringToPath(xrInstance, "/user/hand/right", &xrHandPaths[Hand::Right]), TXT("create path"));
	}

	// generic
	XrPath inputAimPosePath[Hand::MAX];
	XrPath inputGripPosePath[Hand::MAX];
	XrPath inputAClickPath[Hand::MAX];
	XrPath inputBClickPath[Hand::MAX];
	XrPath inputXClickPath[Hand::MAX];
	XrPath inputYClickPath[Hand::MAX];
	XrPath inputATouchPath[Hand::MAX];
	XrPath inputBTouchPath[Hand::MAX];
	XrPath inputXTouchPath[Hand::MAX];
	XrPath inputYTouchPath[Hand::MAX];
	XrPath inputSelectClickPath[Hand::MAX];
	XrPath inputBackClickPath[Hand::MAX];
	XrPath inputMenuClickPath[Hand::MAX];
	XrPath inputMenuTouchPath[Hand::MAX];
	XrPath inputSystemClickPath[Hand::MAX];
	XrPath inputSystemTouchPath[Hand::MAX];
	XrPath inputGripSqueezeClickPath[Hand::MAX];
	XrPath inputGripSqueezeValuePath[Hand::MAX];
	XrPath inputGripSqueezeTouchPath[Hand::MAX];
	XrPath inputTriggerClickPath[Hand::MAX];
	XrPath inputTriggerValuePath[Hand::MAX];
	XrPath inputTriggerTouchPath[Hand::MAX];
	XrPath inputThumbstickPath[Hand::MAX];
	XrPath inputThumbstickClickPath[Hand::MAX];
	XrPath inputThumbstickTouchPath[Hand::MAX];
	XrPath inputTrackpadPath[Hand::MAX];
	XrPath inputTrackpadClickPath[Hand::MAX];
	XrPath inputTrackpadForcePath[Hand::MAX];
	XrPath inputTrackpadTouchPath[Hand::MAX];
	XrPath inputThumbRestTouchPath[Hand::MAX];
	XrPath outputHapticPath[Hand::MAX];
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/aim/pose", &inputAimPosePath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/aim/pose", &inputAimPosePath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/grip/pose", &inputGripPosePath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/grip/pose", &inputGripPosePath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/a/click", &inputAClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/a/click", &inputAClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/b/click", &inputBClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/b/click", &inputBClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/x/click", &inputXClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/x/click", &inputXClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/y/click", &inputYClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/y/click", &inputYClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/a/touch", &inputATouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/a/touch", &inputATouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/b/touch", &inputBTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/b/touch", &inputBTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/x/touch", &inputXTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/x/touch", &inputXTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/y/touch", &inputYTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/y/touch", &inputYTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/select/click", &inputSelectClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/select/click", &inputSelectClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/back/click", &inputBackClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/back/click", &inputBackClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/menu/click", &inputMenuClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/menu/click", &inputMenuClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/menu/touch", &inputMenuTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/menu/touch", &inputMenuTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/system/click", &inputSystemClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/system/click", &inputSystemClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/system/touch", &inputSystemTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/system/touch", &inputSystemTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/squeeze/click", &inputGripSqueezeClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/squeeze/click", &inputGripSqueezeClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/squeeze/value", &inputGripSqueezeValuePath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/squeeze/value", &inputGripSqueezeValuePath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/squeeze/touch", &inputGripSqueezeTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/squeeze/touch", &inputGripSqueezeTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/trigger/click", &inputTriggerClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/trigger/click", &inputTriggerClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/trigger/value", &inputTriggerValuePath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/trigger/value", &inputTriggerValuePath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/trigger/touch", &inputTriggerTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/trigger/touch", &inputTriggerTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/thumbstick", &inputThumbstickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/thumbstick", &inputThumbstickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/thumbstick/click", &inputThumbstickClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/thumbstick/click", &inputThumbstickClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/thumbstick/touch", &inputThumbstickTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/thumbstick/touch", &inputThumbstickTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/thumbrest/touch", &inputThumbRestTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/thumbrest/touch", &inputThumbRestTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/trackpad", &inputTrackpadPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/trackpad", &inputTrackpadPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/trackpad/force", &inputTrackpadForcePath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/trackpad/force", &inputTrackpadForcePath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/trackpad/click", &inputTrackpadClickPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/trackpad/click", &inputTrackpadClickPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/input/trackpad/touch", &inputTrackpadTouchPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/input/trackpad/touch", &inputTrackpadTouchPath[Hand::Right]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/left/output/haptic", &outputHapticPath[Hand::Left]));
	check_openxr(xrStringToPath(xrInstance, "/user/hand/right/output/haptic", &outputHapticPath[Hand::Right]));

	// create action set
	{
		XrActionSetCreateInfo actionSetInfo;
		memory_zero(actionSetInfo);
		actionSetInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
		actionSetInfo.next = nullptr;
		actionSetInfo.priority = 0;
		tstrcpy(actionSetInfo.actionSetName, "gameplay");
		tstrcpy(actionSetInfo.localizedActionSetName, "Gameplay");

		if (!check_openxr(xrCreateActionSet(xrInstance, &actionSetInfo, &xrActionSet), TXT("create action set")))
		{
			isOk = false;
			return;
		}
	}

	struct CreateAction
	{
		OpenXRImpl const* openXR = nullptr;
		XrActionSet xrActionSet = XR_NULL_HANDLE;
		XrPath* xrHandPaths = nullptr;

		void create(OUT_ XrAction& _xrAction, XrActionType _xrActionType, char const* _actionName, char const* _localizedActionName)
		{
			XrActionCreateInfo actionInfo;
			memory_zero(actionInfo);
			actionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
			actionInfo.next = nullptr;
			actionInfo.actionType = _xrActionType;
			actionInfo.countSubactionPaths = Hand::MAX;
			actionInfo.subactionPaths = xrHandPaths;
			tstrcpy(actionInfo.actionName, _actionName);
			tstrcpy(actionInfo.localizedActionName, _localizedActionName);

			openXR->check_openxr(xrCreateAction(xrActionSet, &actionInfo, &_xrAction), TXT("create action"));
		}
	} createAction;
	createAction.openXR = this;
	createAction.xrActionSet = xrActionSet;
	createAction.xrHandPaths = xrHandPaths;

	{
		createAction.create(xrHandAimPoseAction, XR_ACTION_TYPE_POSE_INPUT, "handaimpose", "Hand Aim Pose");
		createAction.create(xrHandGripPoseAction, XR_ACTION_TYPE_POSE_INPUT, "handgrippose", "Hand Grip Pose");
		createAction.create(xrTriggerPressAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "triggerclick", "Trigger Click");
		createAction.create(xrTriggerPressAsFloatAction, XR_ACTION_TYPE_FLOAT_INPUT, "triggerpress", "Trigger Press");
		createAction.create(xrTriggerTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "triggertouch", "Trigger Touch");
		createAction.create(xrGripPressAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "gripclick", "Grip Click");
		createAction.create(xrGripPressAsFloatAction, XR_ACTION_TYPE_FLOAT_INPUT, "grippress", "Grip Press");
		createAction.create(xrGripTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "griptouch", "Grip Touch");
		createAction.create(xrMenuPressAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "menuclick", "Menu Click");
		createAction.create(xrMenuTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "menutouch", "Menu Touch");
		createAction.create(xrPrimaryPressAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "primaryclick", "Primary Click");
		createAction.create(xrPrimaryTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "primarytouch", "Primary Touch");
		createAction.create(xrSecondaryPressAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "secondaryclick", "Secondary Click");
		createAction.create(xrSecondaryTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "secondarytouch", "Secondary Touch");
		createAction.create(xrThumbRestTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumbresttouch", "Thumb Rest Touch");
		createAction.create(xrThumbstickAsVector2Action, XR_ACTION_TYPE_VECTOR2F_INPUT, "thumbstick", "Thumbstick");
		createAction.create(xrThumbstickPressAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumbstickclick", "Thumbstick Click");
		createAction.create(xrThumbstickTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumbsticktouch", "Thumbstick Touch");
		createAction.create(xrTrackpadAsVector2Action, XR_ACTION_TYPE_VECTOR2F_INPUT, "trackpad", "Trackpad");
		createAction.create(xrTrackpadPressAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "trackpadclick", "Trackpad Click");
		createAction.create(xrTrackpadTouchAsBoolAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "trackpadtouch", "Trackpad Touch");

		createAction.create(xrHapticOutputAction, XR_ACTION_TYPE_VIBRATION_OUTPUT, "haptic", "Haptic Vibration");
	}

	struct Binding
	{
		static XrActionSuggestedBinding make(XrAction const& _action, XrPath const& _path)
		{
			XrActionSuggestedBinding binding;
			binding.action = _action;
			binding.binding = _path;
			return binding;
		}
	};

	// suggest actions for different controllers
	{
		check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/khr/simple_controller", &xrInputProfileGeneric));
		check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/oculus/touch_controller", &xrInputProfileOculusTouch));
		check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/valve/index_controller", &xrInputProfileValveIndex));
		check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/htc/vive_controller", &xrInputProfileHTCVive));
		check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/microsoft/motion_controller", &xrInputProfileMicrosoftMotion));
		if (extensionsInUse.does_contain(XR_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME))
		{
			check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/hp/mixed_reality_controller", &xrInputProfileHPMixedReality));
		}
		if (extensionsInUse.does_contain(XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME))
		{
			check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/htc/vive_cosmos_controller", &xrInputProfileHTCViveCosmos));
		}
		if (extensionsInUse.does_contain(XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME))
		{
			check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/htc/vive_focus3_controller", &xrInputProfileHTCViveFocus3));
		}
		if (extensionsInUse.does_contain(XR_PICO_CONTROLLER_INTERACTION_EXTENSION_NAME))
		{
			check_openxr(xrStringToPath(xrInstance, "/interaction_profiles/pico/neo3_controller", &xrInputProfilePicoNeo3Controller));  // used also for pico 4
		}

		// simple controller, just trigger and menu only?
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputMenuClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputMenuClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsBoolAction, inputSelectClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsBoolAction, inputSelectClickPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileGeneric;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - generic"));
		}

		// oculus touch
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputMenuClickPath[Hand::Left]));
			//bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputSystemClickPath[Hand::Right])); reserved for system?
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputXClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputAClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrPrimaryTouchAsBoolAction, inputXTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryTouchAsBoolAction, inputATouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputYClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputBClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryTouchAsBoolAction, inputYTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryTouchAsBoolAction, inputBTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbRestTouchAsBoolAction, inputThumbRestTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbRestTouchAsBoolAction, inputThumbRestTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileOculusTouch;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - oculus touch"));
		}

		// valve index
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			/*
			// not accessible
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputSystemClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputSystemClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuTouchAsBoolAction, inputSystemTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrMenuTouchAsBoolAction, inputSystemTouchPath[Hand::Right]));
			*/
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputAClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputAClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrPrimaryTouchAsBoolAction, inputATouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryTouchAsBoolAction, inputATouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputBClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputBClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryTouchAsBoolAction, inputBTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryTouchAsBoolAction, inputBTouchPath[Hand::Right]));
			//bindings.push_back(Binding::make(xrTriggerPressAsBoolAction, inputTriggerClickPath[Hand::Left]));
			//bindings.push_back(Binding::make(xrTriggerPressAsBoolAction, inputTriggerClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadAsVector2Action, inputTrackpadPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadAsVector2Action, inputTrackpadPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadPressAsBoolAction, inputTrackpadForcePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadPressAsBoolAction, inputTrackpadForcePath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadTouchAsBoolAction, inputTrackpadTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadTouchAsBoolAction, inputTrackpadTouchPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileValveIndex;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - valve index"));
		}

		// htc vive controller
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			/*
			// not accessible at all
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputSystemClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputSystemClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuTouchAsBoolAction, inputSystemTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrMenuTouchAsBoolAction, inputSystemTouchPath[Hand::Right]));
			*/
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputMenuClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputMenuClickPath[Hand::Right]));
			//bindings.push_back(Binding::make(xrTriggerPressAsBoolAction, inputTriggerClickPath[Hand::Left]));
			//bindings.push_back(Binding::make(xrTriggerPressAsBoolAction, inputTriggerClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadAsVector2Action, inputTrackpadPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadAsVector2Action, inputTrackpadPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadPressAsBoolAction, inputTrackpadClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadPressAsBoolAction, inputTrackpadClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadTouchAsBoolAction, inputTrackpadTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadTouchAsBoolAction, inputTrackpadTouchPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileHTCVive;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - htc vive"));
		}

		// microsoft mixed reality motion controller
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputMenuClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputMenuClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadAsVector2Action, inputTrackpadPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadAsVector2Action, inputTrackpadPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadPressAsBoolAction, inputTrackpadClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadPressAsBoolAction, inputTrackpadClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTrackpadTouchAsBoolAction, inputTrackpadTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTrackpadTouchAsBoolAction, inputTrackpadTouchPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileMicrosoftMotion;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - microsoft motion"));
		}

		// hp mixed reality controller (hp reverb g2 controller)
		if (extensionsInUse.does_contain(XR_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME))
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputXClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputAClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputYClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputBClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputMenuClickPath[Hand::Left]));
			//bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputMenuClickPath[Hand::Right])); // system?
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileHPMixedReality;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - hp mixed reality"));
		}

		// htc vive cosmos controller
		if (extensionsInUse.does_contain(XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME))
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputMenuClickPath[Hand::Left]));
			//bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputSystemClickPath[Hand::Right])); reserved for system?
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputXClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputAClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputYClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputBClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileHTCViveCosmos;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - htc vive cosmos"));
		}

		// htc vive focus3 controller
		if (extensionsInUse.does_contain(XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME))
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputMenuClickPath[Hand::Left]));
			//bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputSystemClickPath[Hand::Right])); reserved for system?
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputXClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputAClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputYClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputBClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripTouchAsBoolAction, inputGripSqueezeTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripTouchAsBoolAction, inputGripSqueezeTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbRestTouchAsBoolAction, inputThumbRestTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbRestTouchAsBoolAction, inputThumbRestTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfileHTCViveFocus3;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - htc vive focus 3"));
		}

		// pico neo3 controller
		if (extensionsInUse.does_contain(XR_PICO_CONTROLLER_INTERACTION_EXTENSION_NAME))
		{
			Array<XrActionSuggestedBinding> bindings;
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandAimPoseAction, inputAimPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Left]));
			bindings.push_back(Binding::make(xrHandGripPoseAction, inputGripPosePath[Hand::Right]));
			bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputBackClickPath[Hand::Left]));
			//bindings.push_back(Binding::make(xrMenuPressAsBoolAction, inputBackClickPath[Hand::Right])); this is screenshot button
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputXClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrPrimaryPressAsBoolAction, inputAClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputYClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrSecondaryPressAsBoolAction, inputBClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerPressAsFloatAction, inputTriggerValuePath[Hand::Right]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrTriggerTouchAsBoolAction, inputTriggerTouchPath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsBoolAction, inputGripSqueezeClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Left]));
			bindings.push_back(Binding::make(xrGripPressAsFloatAction, inputGripSqueezeValuePath[Hand::Right]));
			//bindings.push_back(Binding::make(xrGripTouchAsBoolAction, inputGripSqueezeTouchPath[Hand::Left])); // ?
			//bindings.push_back(Binding::make(xrGripTouchAsBoolAction, inputGripSqueezeTouchPath[Hand::Right])); // ?
			//bindings.push_back(Binding::make(xrThumbRestTouchAsBoolAction, inputThumbRestTouchPath[Hand::Left])); // ?
			//bindings.push_back(Binding::make(xrThumbRestTouchAsBoolAction, inputThumbRestTouchPath[Hand::Right])); // ?
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickAsVector2Action, inputThumbstickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickPressAsBoolAction, inputThumbstickClickPath[Hand::Right]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Left]));
			bindings.push_back(Binding::make(xrThumbstickTouchAsBoolAction, inputThumbstickTouchPath[Hand::Right]));

			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Left]));
			bindings.push_back(Binding::make(xrHapticOutputAction, outputHapticPath[Hand::Right]));

			XrInteractionProfileSuggestedBinding suggestedBindings;
			memory_zero(suggestedBindings);
			suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
			suggestedBindings.next = nullptr;
			suggestedBindings.interactionProfile = xrInputProfilePicoNeo3Controller;
			suggestedBindings.countSuggestedBindings = bindings.get_size();
			suggestedBindings.suggestedBindings = bindings.get_data();

			check_openxr(xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings), TXT("suggest bindings - pico neo3 controller"));  // used also for pico 4
		}
	}

	// add
	//	/interaction_profiles/valve/index_controller
	//	and more!

	// attach to session
	{
		XrSessionActionSetsAttachInfo actionSetAttachInfo;
		memory_zero(actionSetAttachInfo);
		actionSetAttachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
		actionSetAttachInfo.next = nullptr;
		actionSetAttachInfo.countActionSets = 1;
		actionSetAttachInfo.actionSets = &xrActionSet;
		if (!check_openxr(xrAttachSessionActionSets(xrSession, &actionSetAttachInfo), TXT("attach action set")))
		{
			isOk = false;
			return;
		}
	}

	// to query hand pose, space has to be create for each one
	// these are created here as the session has to be running
	{
		for_count(int, handIdx, Hand::MAX)
		{
			if (xrHandAimPoseSpaces[handIdx] == XR_NULL_HANDLE)
			{
				XrActionSpaceCreateInfo actionSpaceInfo;
				memory_zero(actionSpaceInfo);
				actionSpaceInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
				actionSpaceInfo.next = nullptr;
				actionSpaceInfo.action = xrHandAimPoseAction;
				actionSpaceInfo.poseInActionSpace = xrIdentityPose;
				actionSpaceInfo.subactionPath = xrHandPaths[handIdx];

				check_openxr(xrCreateActionSpace(xrSession, &actionSpaceInfo, &xrHandAimPoseSpaces[handIdx]), TXT("create action space hand aim"));
			}
		}

		for_count(int, handIdx, Hand::MAX)
		{
			if (xrHandGripPoseSpaces[handIdx] == XR_NULL_HANDLE)
			{
				XrActionSpaceCreateInfo actionSpaceInfo;
				memory_zero(actionSpaceInfo);
				actionSpaceInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
				actionSpaceInfo.next = nullptr;
				actionSpaceInfo.action = xrHandGripPoseAction;
				actionSpaceInfo.poseInActionSpace = xrIdentityPose;
				actionSpaceInfo.subactionPath = xrHandPaths[handIdx];

				check_openxr(xrCreateActionSpace(xrSession, &actionSpaceInfo, &xrHandGripPoseSpaces[handIdx]), TXT("create action space hand grip"));
			}
		}
	}

	// mark we're using these devices
	{
		handControllerTrackDevices[Hand::Left].controllerType = Hand::Left;
		handControllerTrackDevices[Hand::Left].deviceID = Hand::Left;
		handControllerTrackDevices[Hand::Left].hand = Hand::Left;
		handControllerTrackDevices[Hand::Right].controllerType = Hand::Right;
		handControllerTrackDevices[Hand::Right].deviceID = Hand::Right;
		handControllerTrackDevices[Hand::Right].hand = Hand::Right;
	}

	if (ext_xrSetColorSpaceFB)
	{
		{
			PFN_xrEnumerateColorSpacesFB ext_xrEnumerateColorSpacesFB = nullptr;
			load_ext_function("xrEnumerateColorSpacesFB", &ext_xrEnumerateColorSpacesFB);
			if (ext_xrEnumerateColorSpacesFB)
			{
				uint32_t colorSpaceCount = 0;
				check_openxr(ext_xrEnumerateColorSpacesFB(xrSession, 0, &colorSpaceCount, nullptr), TXT("enumerate colour spaces"));

				Array<XrColorSpaceFB> colorSpaces;
				colorSpaces.set_size(colorSpaceCount);

				check_openxr(ext_xrEnumerateColorSpacesFB(xrSession, colorSpaceCount, &colorSpaceCount, colorSpaces.begin()), TXT("enumerate colour spaces [2]"));

				/*
				output(TXT("[system-openxr] available color spaces"));
				for_every(cs, colorSpaces)
				{
					output(TXT("- %i"), *cs);
				}
				*/
			}
		}

		//XrColorSpaceFB const requestColorSpace = XR_COLOR_SPACE_UNMANAGED_FB;
		XrColorSpaceFB const requestColorSpace = XR_COLOR_SPACE_REC2020_FB;
		//XrColorSpaceFB const requestColorSpace = XR_COLOR_SPACE_RIFT_CV1_FB;

		output(TXT("[system-openxr] set color space %i"), requestColorSpace);

		check_openxr(ext_xrSetColorSpaceFB(xrSession, requestColorSpace), TXT("request color space"));
	}

	if (ext_xrGetDisplayRefreshRateFB)
	{
		float drr = 0.0f;
		if (check_openxr(ext_xrGetDisplayRefreshRateFB(xrSession, &drr), TXT("get display refresh rate")))
		{
			if (drr == 0.0f)
			{
				output(TXT("[system-openxr] display refresh rate not available (returned %.1f)"), drr);
			}
			else
			{
				displayRefreshRate = drr;
				output(TXT("[system-openxr] display refresh rate %.1f"), displayRefreshRate);
			}
		}
	}

	create_hand_trackers();

	enteredVRModeTS.set_if_not_set();
	enteredVRModeTS.access().reset();
}

void OpenXRImpl::create_hand_trackers()
{
#ifdef AN_OPENXR_WITH_HAND_TRACKING
	handTrackingInitialised = true;

	close_hand_trackers();

	if (!handTrackingAvailable)
	{
		return;
	}

	for_count(int, handIdx, Hand::MAX)
	{
		XrHandTrackerCreateInfoEXT createHandTrackerInfo;
		memory_zero(createHandTrackerInfo);
		createHandTrackerInfo.type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT;
		createHandTrackerInfo.hand = handIdx == Hand::Left? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
		createHandTrackerInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		if (!ext_xrCreateHandTrackerEXT ||
			!check_openxr(ext_xrCreateHandTrackerEXT(xrSession, &createHandTrackerInfo, &xrHandTrackers[handIdx]), TXT("create hand tracker")))
		{
			handTrackingInitialised = false;
		}
		else
		{
			if (handTrackingMeshSupported && ext_xrGetHandMeshFB)
			{
				int jointCount = 0;
				int vertexCount = 0;
				int indexCount = 0;
				{
					XrHandTrackingMeshFB meshInfo;
					memory_zero(meshInfo);
					meshInfo.type = XR_TYPE_HAND_TRACKING_MESH_FB;
					if (check_openxr(ext_xrGetHandMeshFB(xrHandTrackers[handIdx], &meshInfo), TXT("get hand mesh")))
					{
						jointCount = meshInfo.jointCountOutput;
						vertexCount = meshInfo.vertexCountOutput;
						indexCount = meshInfo.indexCountOutput;
					}
				}

				output(TXT("[system-openxr] hand tracking mesh joint count: %i"), jointCount);

				if (jointCount > 0)
				{
					Array<XrPosef> jointBindBoses;
					Array<float> jointRadii;
					Array<XrHandJointEXT> jointParents;
					jointBindBoses.set_size(jointCount);
					jointRadii.set_size(jointCount);
					jointParents.set_size(jointCount);

					XrHandTrackingMeshFB meshInfo;
					memory_zero(meshInfo);
					meshInfo.type = XR_TYPE_HAND_TRACKING_MESH_FB;
					meshInfo.jointCapacityInput = jointCount;
					meshInfo.jointBindPoses = jointBindBoses.begin();
					meshInfo.jointRadii = jointRadii.begin();
					meshInfo.jointParents = jointParents.begin();

					// we don't need them, but they have to be there to make it possible to read bones alone
					//
					Array<XrVector3f> vertexPositions;
					Array<XrVector3f> vertexNormals;
					Array<XrVector2f> vertexUVs;
					Array<XrVector4sFB> vertexBlendIndices;
					Array<XrVector4f> vertexBlendWeights;
					vertexPositions.set_size(vertexCount);
					vertexNormals.set_size(vertexCount);
					vertexUVs.set_size(vertexCount);
					vertexBlendIndices.set_size(vertexCount);
					vertexBlendWeights.set_size(vertexCount);
					meshInfo.vertexCapacityInput = vertexCount;
					meshInfo.vertexPositions = vertexPositions.begin();
					meshInfo.vertexNormals = vertexNormals.begin();
					meshInfo.vertexUVs = vertexUVs.begin();
					meshInfo.vertexBlendIndices = vertexBlendIndices.begin();
					meshInfo.vertexBlendWeights = vertexBlendWeights.begin();
					//
					Array<int16_t> indices;
					indices.set_size(indexCount);
					meshInfo.indexCapacityInput = indexCount;
					meshInfo.indices = indices.begin();

					if (check_openxr(ext_xrGetHandMeshFB(xrHandTrackers[handIdx], &meshInfo), TXT("get hand mesh")))
					{
						auto& refHandPose = owner->access_reference_pose_set().hands[handIdx];
						Transform handPose = get_transform_from(jointBindBoses[XR_HAND_JOINT_PALM_EXT]);
						refHandPose.rawPlacement = handPose;
						refHandPose.rawHandTrackingRootPlacement = handPose;
						for_count(int, b, jointCount)
						{
							refHandPose.rawBonesRS[b] = handPose.to_local(get_transform_from(jointBindBoses[b]));
							refHandPose.rawBoneParents[b] = jointParents[b];
							if (refHandPose.rawBoneParents[b] < 0 || refHandPose.rawBoneParents[b] > jointCount)
							{
								refHandPose.rawBoneParents[b] = NONE;
							}
						}

						refHandPose.calculate_auto_and_add_offsets((Hand::Type)handIdx, true);
					}
				}
			}
		}
	}
#endif
}

void OpenXRImpl::close_hand_trackers()
{
#ifdef AN_OPENXR_WITH_HAND_TRACKING
	for_count(int, handIdx, Hand::MAX)
	{
		auto& xrHandTracker = xrHandTrackers[handIdx];
		if (xrHandTracker != XR_NULL_HANDLE)
		{
			if (ext_xrDestroyHandTrackerEXT)
			{
				ext_xrDestroyHandTrackerEXT(xrHandTracker);
			}
			xrHandTracker = XR_NULL_HANDLE;
		}
	}	
#endif
}

void OpenXRImpl::close_device_impl()
{
	enteredVRModeTS.clear();

	close_internal_render_targets();

	close_hand_trackers();

	for_count(int, handIdx, Hand::MAX)
	{
		if (xrHandAimPoseSpaces[handIdx] != XR_NULL_HANDLE) { xrDestroySpace(xrHandAimPoseSpaces[handIdx]); xrHandAimPoseSpaces[handIdx] = XR_NULL_HANDLE; }
		if (xrHandGripPoseSpaces[handIdx] != XR_NULL_HANDLE) { xrDestroySpace(xrHandGripPoseSpaces[handIdx]); xrHandGripPoseSpaces[handIdx] = XR_NULL_HANDLE; }
	}

	if (xrActionSet != XR_NULL_HANDLE) { xrDestroyActionSet(xrActionSet); }

	if (xrPlaySpace != XR_NULL_HANDLE) { xrDestroySpace(xrPlaySpace); xrPlaySpace = XR_NULL_HANDLE; }
	if (xrViewPoseSpace != XR_NULL_HANDLE) { xrDestroySpace(xrViewPoseSpace); xrViewPoseSpace = XR_NULL_HANDLE; }

	if (ext_xrDestroyDebugUtilsMessengerEXT &&
		xrDebug != XR_NULL_HANDLE)
	{
		ext_xrDestroyDebugUtilsMessengerEXT(xrDebug);
		xrDebug = XR_NULL_HANDLE; 
	}

	if (xrSession != XR_NULL_HANDLE)
	{
		xrDestroySession(xrSession);
		xrSession = XR_NULL_HANDLE;
	}

	// reset session state variables
	xrSessionRunning = false;
	xrSessionSynchronised = false;
	xrSessionInFocus = false;
	xrSessionVisible = false;
}

bool OpenXRImpl::can_load_play_area_rect_impl()
{
	static bool triedBefore = false;
	bool firstTry = !triedBefore;
	triedBefore = true;
	if (firstTry)
	{
		output(TXT("[system-openxr] check if can load play area rect"));
	}

	if (xrSession == XR_NULL_HANDLE)
	{
		if (firstTry)
		{
			output(TXT("[system-openxr] no session"));
		}
		todo_note(TXT("message why - no device?"));
		return false;
	}

	if (firstTry)
	{
		output(TXT("[system-openxr] get calibration state"));
	}

	XrExtent2Df xrBounds;
	XrResult result = xrGetReferenceSpaceBoundsRect(xrSession, referenceSpaceType, &xrBounds);
	if (result == XR_SPACE_BOUNDS_UNAVAILABLE)
	{
		warn(TXT("[system-openxr] not calibrated"));
		return false;
	}
	if (! XR_SUCCEEDED(result))
	{
		warn_dev_investigate(TXT("could not load bounds"));
		return false;
	}

	xrBounds.width = abs(xrBounds.width);
	xrBounds.height = abs(xrBounds.height);

	sanitise_xr_bounds_size(xrBounds);

	if (xrBounds.width  > MAX_VR_SIZE ||
		xrBounds.height > MAX_VR_SIZE ||
		xrBounds.width  < 0.000001f ||
		xrBounds.height < 0.000001f)
	{
		owner->add_warning(String(TXT("Could not get proper boundary dimension. Please check device and up.")));
		output(TXT("[system-openxr] couldn't get proper boundary dimensions"));
		return false;
	}
	return true;
}

bool OpenXRImpl::load_play_area_rect_impl(bool _loadAnything)
{
	XrExtent2Df xrBounds;
	XrResult result = xrGetReferenceSpaceBoundsRect(xrSession, referenceSpaceType, &xrBounds);
	if (result == XR_SPACE_BOUNDS_UNAVAILABLE ||
		! XR_SUCCEEDED(result))
	{
		warn(TXT("[system-openxr] play area could not be read at the time (%i:%S)"), result, result == XR_SPACE_BOUNDS_UNAVAILABLE ? TXT("space bounds unavailable") : TXT("something else"));
#ifdef AN_INSPECT_VR_PLAY_AREA
		output(TXT("[system-openxr] play area could not be read at the time (%i:%S)"), result, result == XR_SPACE_BOUNDS_UNAVAILABLE? TXT("space bounds unavailable") : TXT("something else"));
#endif
		boundaryUnavailable = true;
		if (_loadAnything)
		{
#ifdef AN_INSPECT_VR_PLAY_AREA
			output(TXT("[system-openxr] load anything -> results in a warning"));
#endif
			owner->set_play_area_rect(Vector3::yAxis * 1.2f * 0.5f, Vector3::xAxis * 1.8f * 0.5f);
			owner->add_warning(String(TXT("Calibrate to get the right play area.")));
			return true;
		}
		else
		{
#ifdef AN_INSPECT_VR_PLAY_AREA
			output(TXT("[system-openxr] we require some result -> results in an error"));
#endif
			owner->add_error(String(TXT("Calibrate to get the right play area.")));
			return false;
		}
	}
	
	boundaryUnavailable = false; // available!

	Transform origin = Transform::identity;
	owner->set_map_vr_space_origin(origin);

#ifdef AN_INSPECT_VR_PLAY_AREA
	output(TXT("[system-openxr] play area centre read %.3fx%.3f (yaw %.0f)"), origin.get_translation().x, origin.get_translation().y, origin.get_orientation().to_rotator().yaw);
#endif

	// dimensions after origin

#ifdef AN_INSPECT_VR_PLAY_AREA
	output(TXT("[system-openxr] play area size read %.3fx%.3f"), xrBounds.width, xrBounds.height);
#endif

	sanitise_xr_bounds_size(xrBounds);

	Vector3 dimensions = Vector3(xrBounds.width, xrBounds.height, 0.0f);

	dimensions = owner->get_map_vr_space_auto().vector_to_local(dimensions);

	// make them absolute
	dimensions.x = abs(dimensions.x);
	dimensions.y = abs(dimensions.y);

	auto rawDimensions = dimensions;

	// make values absolute and clamp
	dimensions.x = clamp(dimensions.x, 0.1f, MAX_VR_SIZE);
	dimensions.y = clamp(dimensions.y, 0.1f, MAX_VR_SIZE);

	owner->set_play_area_rect(dimensions * Vector3::yAxis * 0.5f, dimensions * Vector3::xAxis * 0.5f);

	Array<Vector2> rawBoundaryPoints;

#ifdef XR_USE_GET_BOUND_2D
	// returns invalid handle 
	if (ext_xrGetSpaceBoundary2DFB)
	{
		if (xrPlaySpace == XR_NULL_HANDLE)
		{
			warn(TXT("[system-openxr] no play space available"));
		}
		else
		{
			XrBoundary2DFB boundaryInfo;
			memory_zero(boundaryInfo);
			boundaryInfo.type = XR_TYPE_BOUNDARY_2D_FB;
			boundaryInfo.next = nullptr;

			if (check_openxr_warn(ext_xrGetSpaceBoundary2DFB(xrSession, xrPlaySpace, &boundaryInfo)))
			{
				Array<XrVector2f> xrPoints;
				xrPoints.set_size(boundaryInfo.vertexCountOutput);

				boundaryInfo.vertexCapacityInput = xrPoints.get_size();
				boundaryInfo.vertexCountOutput = 0;
				boundaryInfo.vertices = xrPoints.begin();

				if (check_openxr_warn(ext_xrGetSpaceBoundary2DFB(xrSession, xrPlaySpace, &boundaryInfo)))
				{
					rawBoundaryPoints.clear();
					rawBoundaryPoints.make_space_for(xrPoints.get_size());
					for_every(p, xrPoints)
					{
						rawBoundaryPoints.push_back(get_vector2_from(*p));
					}
				}
			}
			else
			{
				warn(TXT("[system-openxr] unable to get boundary"));
			}
		}
	}
#endif

	if ((dimensions - rawDimensions).is_almost_zero())
	{
		// read boundary
		owner->set_raw_boundary(rawBoundaryPoints);
	}
	else
	{
		owner->set_raw_boundary(rawBoundaryPoints);
		owner->set_map_vr_space_origin(Transform::identity);
	}

	output(TXT("[system-openxr] origin: at %.3fx%.3f (+%.3f) (y:%.3f)"), origin.get_translation().x, origin.get_translation().y, origin.get_translation().z, origin.get_orientation().to_rotator().yaw);
	output(TXT("[system-openxr] half right %.3f (full %.3f)"), owner->get_raw_whole_play_area_rect_half_right().length(), dimensions.x);
	output(TXT("[system-openxr] half forward %.3f (full %.3f)"), owner->get_raw_whole_play_area_rect_half_forward().length(), dimensions.y);

	owner->act_on_possibly_invalid_boundary();

	if (owner->get_raw_whole_play_area_rect_half_right().length() >= owner->get_raw_whole_play_area_rect_half_forward().length())
	{
		return true;
	}

	return false;
}

void OpenXRImpl::init_hand_controller_track_device(int _index)
{
	// not used
}

void OpenXRImpl::close_hand_controller_track_device(int _index)
{
	// not used
}

void OpenXRImpl::update_track_device_indices()
{
	// not used
}

void OpenXRImpl::advance_events()
{
#ifdef AN_OPEN_XR_EXTENDED_DEBUG
	output(TXT("[system-openxr] OpenXRImpl::advance_events begin"));
#endif

#ifdef AN_ANDROID
	{
		auto& questApp = System::Android::App::get();
		// process all events once
		{
			scoped_call_stack_info(TXT("process events, quickly"));
			questApp.process_events();
		}
	}
#endif

	// events
	XrEventDataBuffer runtimeEvent;
	memory_zero(runtimeEvent);
	runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
	runtimeEvent.next = nullptr;

	bool reinit = false;

	XrResult pollResult = XR_SUCCESS;
#ifdef AN_OPEN_XR_EXTENDED_DEBUG
#endif
	while (true)
	{
		pollResult = xrPollEvent(xrInstance, &runtimeEvent);
		if (pollResult != XR_SUCCESS)
		{
			break;
		}

		switch (runtimeEvent.type)
		{
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
			{
				output(TXT("[system-openxr] EVENT: reference space change pending"));
				if (!enteredVRModeTS.is_set() ||
					enteredVRModeTS.get().get_time_since() < 1.0f)
				{
					output(TXT("[system-openxr]  not initialised or too soon"));
				}
				else
				{
					output(TXT("[system-openxr]  react to reference space change"));
					recenterPending = 1;

					todo_hack(TXT("recentering hacks required as reset view changes whole space for no reason"));

#ifdef USE_HACKY_WORKAROUND_FOR_RECENTER__RECREATE_SESSION_FOR_RECENTER
					recenterPending = 2; // this is required to make it work properly with hub
					reinit = true;
#else 
#ifdef USE_HACKY_WORKAROUND_FOR_RECENTER__OFFSET_BY_DIFF
					if (owner->get_controls_pose_set().rawViewAsRead.is_set())
					{
						Transform viewVR = owner->get_controls_pose_set().rawViewAsRead.get();
						Vector3 forwardVR = calculate_flat_forward_from(viewVR);

						Transform placementVR = look_matrix(viewVR.get_translation() * Vector3::xy, forwardVR, Vector3::zAxis).to_transform();

						owner->offset_additional_offset_to_raw_as_read(placementVR);
					}
#else
					XrEventDataReferenceSpaceChangePending* event = (XrEventDataReferenceSpaceChangePending*)&runtimeEvent;
					if (event->poseValid)
					{
						Transform newOrigin = get_transform_from(event->poseInPreviousSpace).inverted();
						output(TXT("[system-openxr] pending reference space change (pose in previous space available)"));
						owner->set_map_vr_space_origin(newOrigin);
					}
					else
					{
						output(TXT("[system-openxr] pending reference space change (hard reset)"));
						owner->set_map_vr_space_origin(Transform::identity);
					}
#endif
#endif
					/*
					{
						output(TXT("[system-openxr] recreate xrPlaySpace"));
						if (xrPlaySpace != XR_NULL_HANDLE) { xrDestroySpace(xrPlaySpace); xrPlaySpace = XR_NULL_HANDLE; }
						{
							XrReferenceSpaceCreateInfo playSpaceCreateInfo;
							memory_zero(playSpaceCreateInfo);
							playSpaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
							playSpaceCreateInfo.next = nullptr;
							playSpaceCreateInfo.referenceSpaceType = referenceSpaceType;
							playSpaceCreateInfo.poseInReferenceSpace = xrIdentityPose;

							XrResult result = xrCreateReferenceSpace(xrSession, &playSpaceCreateInfo, &xrPlaySpace);
							if (!check_openxr(result, TXT("create play space")))
							{
								isOk = false;
								return;
							}
						}
					}
					*/
				}
				break;
			}
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
			{
				XrEventDataInstanceLossPending* event = (XrEventDataInstanceLossPending*)&runtimeEvent;
				warn(TXT("[system-openxr] EVENT: instance loss pending at %lu! Destroying instance."), event->lossTime);
				::System::Core::quick_exit(true);
				continue;
			}
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
			{
				XrEventDataSessionStateChanged* event = (XrEventDataSessionStateChanged*)&runtimeEvent;
				output(TXT("[system-openxr] EVENT: session state changed from %d to %d"), xrSessionState, event->state);
				if (xrSessionState != event->state)
				{
					xrSessionState = event->state;

					/*
					 * react to session state changes, see OpenXR spec 9.3 diagram. What we need to react to:
					 *
					 * * READY -> xrBeginSession STOPPING -> xrEndSession (note that the same session can be restarted)
					 * * EXITING -> xrDestroySession (EXITING only happens after we went through STOPPING and called xrEndSession)
					 *
					 * After exiting it is still possible to create a new session but we don't do that here.
					 *
					 * * IDLE -> don't run render loop, but keep polling for events
					 * * SYNCHRONIZED, VISIBLE, FOCUSED -> run render loop
					 */
					switch (xrSessionState)
					{
						// skip render loop, keep polling
						case XR_SESSION_STATE_MAX_ENUM: // must be a bug
						case XR_SESSION_STATE_IDLE:
						case XR_SESSION_STATE_UNKNOWN:
							output(TXT("[system-openxr] state-> idle/unknown [%i=%S]"), xrSessionState, xrSessionState == XR_SESSION_STATE_IDLE? TXT("idle") : (xrSessionState == XR_SESSION_STATE_UNKNOWN ? TXT("unknown") : TXT("??")));
							::System::Core::pause_rendering(bit(2));
							::System::Core::pause_vr(bit(2));
							::System::Core::pause_vr(bit(1));
							xrSessionSynchronised = false;
							xrSessionInFocus = false;
							xrSessionVisible = false;
							break;
						case XR_SESSION_STATE_SYNCHRONIZED:
							output(TXT("[system-openxr] state-> synchronized but not visible"));
							::System::Core::pause_rendering(bit(2));
							::System::Core::pause_vr(bit(2));
							::System::Core::pause_vr(bit(1));
							xrSessionSynchronised = true;
							xrSessionInFocus = false;
							xrSessionVisible = false;
							break;
						case XR_SESSION_STATE_VISIBLE:
							output(TXT("[system-openxr] state-> visible but without focus"));
							::System::Core::unpause_rendering(bit(2));
							::System::Core::unpause_vr(bit(2));
							::System::Core::pause_vr(bit(1));
							xrSessionSynchronised = true;
							xrSessionVisible = true;
							xrSessionInFocus = false;
							break;
						case XR_SESSION_STATE_FOCUSED:
							output(TXT("[system-openxr] state-> focus gained"));
							::System::Core::unpause_rendering(bit(2));
							::System::Core::unpause_vr(bit(2));
							::System::Core::unpause_vr(bit(1));
							xrSessionSynchronised = true;
							xrSessionInFocus = true;
							xrSessionVisible = true;
							break;
						case XR_SESSION_STATE_READY:
							output(TXT("[system-openxr] state-> ready"));
							if (!xrSessionRunning)
							{
								XrSessionBeginInfo sessionBeginInfo;
								memory_zero(sessionBeginInfo);
								sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
								sessionBeginInfo.next = nullptr;
								sessionBeginInfo.primaryViewConfigurationType = viewType;
								{
									XrResult result = xrBeginSession(xrSession, &sessionBeginInfo);
									if (!check_openxr(result, TXT("begin session")))
									{
										return;
									}
								}
								output(TXT("[system-openxr] session started"));
								xrSessionRunning = true;

								if (ownFoveationUsed)
								{
									::System::FoveatedRenderingSetup::force_set_foveation();
								}

								// setup session once it starts
								set_processing_levels_impl();
								update_perf_threads();
							}
							::System::Core::unpause_rendering(bit(2));
							::System::Core::unpause_vr(bit(2));
							::System::Core::unpause_vr(bit(1));
							xrSessionSynchronised = true;
							xrSessionVisible = true;
							xrSessionInFocus = true;
							break;
						case XR_SESSION_STATE_STOPPING:
							output(TXT("[system-openxr] state-> stopping"));
							if (xrSessionRunning)
							{
								{
									XrResult result = xrEndSession(xrSession);
									if (!check_openxr(result, TXT("end session")))
									{
										return;
									}
								}
								xrSessionRunning = false;
							}
							::System::Core::pause_rendering(bit(2));
							::System::Core::pause_vr(bit(2));
							::System::Core::pause_vr(bit(1));
							xrSessionSynchronised = false;
							xrSessionInFocus = false;
							xrSessionVisible = false;
							break;
						case XR_SESSION_STATE_LOSS_PENDING:
						case XR_SESSION_STATE_EXITING:
							output(TXT("[system-openxr] state-> exit"));
							::System::Core::quick_exit(true);

							break;
					}
				}
				break;
			}
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
			{
				output(TXT("[system-openxr] EVENT: interaction profile changed"));
				owner->decide_hands();
				break;
			}
			case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB:
			{
				output(TXT("[system-openxr] EVENT: display refresh rate"));
				if (XrEventDataDisplayRefreshRateChangedFB const* refreshRateChangedEvent = (XrEventDataDisplayRefreshRateChangedFB*)(&runtimeEvent))
				{
					displayRefreshRate = refreshRateChangedEvent->toDisplayRefreshRate;
					output(TXT("[system-openxr] display refresh rate is %.1f"), refreshRateChangedEvent->toDisplayRefreshRate);
				}
				break;
			}
			case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
			{
				output(TXT("[system-openxr] EVENT: performance settings"));
				void const * eventData = &runtimeEvent;
				while (eventData)
				{
					if (XrEventDataPerfSettingsEXT const* perfSettings = (XrEventDataPerfSettingsEXT*)(eventData))
					{
						if (perfSettings->type == XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT)
						{
							auto& unit = perfSettings->domain == XR_PERF_SETTINGS_DOMAIN_CPU_EXT ? performanceStats.cpu : performanceStats.gpu;
							auto& what = perfSettings->subDomain == XR_PERF_SETTINGS_SUB_DOMAIN_COMPOSITING_EXT ? unit.compositing :
										(perfSettings->subDomain == XR_PERF_SETTINGS_SUB_DOMAIN_RENDERING_EXT ? unit.rendering : unit.thermal);
							what = perfSettings->toLevel == XR_PERF_SETTINGS_NOTIF_LEVEL_NORMAL_EXT ? 1.0f :
								  (perfSettings->toLevel == XR_PERF_SETTINGS_NOTIF_LEVEL_WARNING_EXT ? 0.75f : 0.25f);
							output(TXT("[system-openxr]   domain: %S"), perfSettings->domain == XR_PERF_SETTINGS_DOMAIN_CPU_EXT ? TXT("CPU") : TXT("GPU"));
							output(TXT("[system-openxr]   subdomain: %S"), perfSettings->subDomain == XR_PERF_SETTINGS_SUB_DOMAIN_COMPOSITING_EXT ? TXT("compositing") :
																		  (perfSettings->subDomain == XR_PERF_SETTINGS_SUB_DOMAIN_RENDERING_EXT ? TXT("rendering") : TXT("thermal")));
							output(TXT("[system-openxr]   from level (%i): %S"), perfSettings->fromLevel,
																		   perfSettings->fromLevel == XR_PERF_SETTINGS_NOTIF_LEVEL_NORMAL_EXT ? TXT("normal") :
																		  (perfSettings->fromLevel == XR_PERF_SETTINGS_NOTIF_LEVEL_WARNING_EXT ? TXT("warning") : TXT("impaired")));
							output(TXT("[system-openxr]   to level (%i): %S"), perfSettings->toLevel,
																		   perfSettings->toLevel == XR_PERF_SETTINGS_NOTIF_LEVEL_NORMAL_EXT ? TXT("normal") :
																		  (perfSettings->toLevel == XR_PERF_SETTINGS_NOTIF_LEVEL_WARNING_EXT ? TXT("warning") : TXT("impaired")));

							log_performance_stats();
						}
					}
					if (XrEventDataBuffer const* eventDataBuffer = (XrEventDataBuffer*)eventData)
					{
						eventData = eventDataBuffer->next;
					}
				}
				break;
			}
			default:
				warn(TXT("[system-openxr] EVENT: unhandled event (type %i))"), runtimeEvent.type);
				break;
		}

		runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
	}

	if (reinit)
	{
		output(TXT("[system-openxr] reinit"));
		close_device_impl();
		enter_vr_mode_impl();
		setup_rendering_on_init_video(::System::Video3D::get());
		bool unused1;
		bool unused2;
		create_internal_render_targets(renderTargetsCreatedForSource, renderTargetsCreatedForFallbackEyeResolution, OUT_ unused1, OUT_ unused2);
	}

	if (xrSessionRunning)
	{
		if (recenterPending > 0)
		{
			-- recenterPending;
			{
				output(TXT("[system-openxr] request recenter"));
				owner->access_controls().requestRecenter = true;
				if (recenterPending == 0)
				{
					output(TXT("[system-openxr] mark pending reset immobile origin in vr space"));
					owner->mark_pending_reset_immobile_origin_in_vr_space();
				}
			}
		}

		if (! displayRefreshRate.is_set() &&
			ext_xrGetDisplayRefreshRateFB)
		{
			float drr = 0.0f;
			if (check_openxr(ext_xrGetDisplayRefreshRateFB(xrSession, &drr), TXT("get display refresh rate")))
			{
				if (drr == 0.0f)
				{
					output(TXT("[system-openxr] display refresh rate not available (returned %.1f)"), drr);
				}
				else
				{
					displayRefreshRate = drr;
					output(TXT("[system-openxr] display refresh rate %.1f"), displayRefreshRate);
				}
			}
		}
	}

#ifdef XR_USE_META_METRICS
	{
		if (ext_xrSetPerformanceMetricsStateMETA)
		{
			XrPerformanceMetricsStateMETA state = { XR_TYPE_PERFORMANCE_METRICS_STATE_META };
			state.enabled = XR_TRUE;
			ext_xrSetPerformanceMetricsStateMETA(xrSession, &state);
		}
	}
#endif
}

void OpenXRImpl::advance_pulses()
{
	for_count(int, handIdx, Hand::MAX)
	{
		auto * pulse = owner->get_pulse(handIdx);

		XrHapticActionInfo hapticActionInfo;
		memory_zero(hapticActionInfo);
		hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
		hapticActionInfo.next = nullptr;
		hapticActionInfo.action = xrHapticOutputAction;
		hapticActionInfo.subactionPath = xrHandPaths[handIdx];

		float const hapticScale = 1.5f; // without it, the default values feel weak
		float currentStrength = xrSessionInFocus && pulse? pulse->get_current_strength() * MainConfig::global().get_vr_haptic_feedback() * hapticScale : 0.0f;
		if (currentStrength > 0.0f)
		{
			XrHapticVibration vibration;
			memory_zero(vibration);
			vibration.type = XR_TYPE_HAPTIC_VIBRATION;
			vibration.next = nullptr;
			vibration.amplitude = clamp(currentStrength, 0.0f, 1.0f); // should not exceed 1.0
			vibration.duration = (XrDuration)(pulse->get_time_left() * 1000000000.0f);
			vibration.frequency = pulse->get_current_frequency();

			check_openxr(xrApplyHapticFeedback(xrSession, &hapticActionInfo, (const XrHapticBaseHeader*)&vibration), TXT("apply haptic"));
			hapticPerHandOn[handIdx] = true;
		}
		else if (hapticPerHandOn[handIdx])
		{
			check_openxr(xrStopHapticFeedback(xrSession, &hapticActionInfo), TXT("stop haptic"));
			hapticPerHandOn[handIdx] = false;
		}
	}
}

void OpenXRImpl::advance_poses()
{
	auto& vrControls = owner->access_controls();
	auto& poseSet = owner->access_controls_pose_set();

	// advance poses
	{
		TrackingState trackingState;
		read_tracking_state(OUT_ trackingState, syncPredictedDisplayTime /*predictedDisplayTime*/);

		store(poseSet, &vrControls, trackingState);
#ifdef DEBUG_READING_POSES
		{
			auto& pose = owner->access_controls_pose_set();
			if (pose.view.is_set())
			{
				output(TXT("[system-openxr] advance controls pose %.3f x %.3f, %.3f"), pose.view.get().get_translation().x, pose.view.get().get_translation().y, pose.view.get().get_orientation().to_rotator().yaw);
			}
		}
#endif
	}
}

void OpenXRImpl::close_internal_render_targets()
{
	for_count(int, rmIdx, InternalRenderMode::COUNT)
	{
		auto& rmd = renderModeData[rmIdx];
		if (rmd.xrFoveationProfile != XR_NULL_HANDLE && ext_xrDestroyFoveationProfileFB)
		{
			ext_xrDestroyFoveationProfileFB(rmd.xrFoveationProfile);
			rmd.xrFoveationProfile = XR_NULL_HANDLE;
		}
		for_every(xsc, rmd.xrSwapchains)
		{
			if (xsc->xrSwapchain != XR_NULL_HANDLE) xrDestroySwapchain(xsc->xrSwapchain);
			if (xsc->xrDepthSwapchain != XR_NULL_HANDLE) xrDestroySwapchain(xsc->xrDepthSwapchain);

			xsc->directToVRRenderTargets.clear();

			for_every(imageFrameBufferID, xsc->imageFrameBufferIDs)
			{
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDeleteFramebuffers(1, imageFrameBufferID); AN_GL_CHECK_FOR_ERROR_ALWAYS
				*imageFrameBufferID = 0;
			}

			for_every(depthRenderBufferID, xsc->depthRenderBufferIDs)
			{
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDeleteRenderbuffers(1, depthRenderBufferID); AN_GL_CHECK_FOR_ERROR_ALWAYS
				*depthRenderBufferID = 0;
			}
		}
	}
}

void OpenXRImpl::create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget)
{
	renderTargetsCreatedForSource = _source;
	renderTargetsCreatedForFallbackEyeResolution = _fallbackEyeResolution;

	close_internal_render_targets();

	//REMOVE_OR_CHANGE_BEFORE_COMMIT_ MainConfig::access_global().access_video().directlyToVR = true;
	//REMOVE_OR_CHANGE_BEFORE_COMMIT_ MainConfig::access_global().access_video().msaa = true;
	//REMOVE_OR_CHANGE_BEFORE_COMMIT_ MainConfig::access_global().access_video().vrFoveatedRenderingLevel = 2.0f;
	_useDirectToVRRendering = MainConfig::global().get_video().directlyToVR;
#ifdef AN_ANDROID
#ifdef AN_VIVE
	_noOutputRenderTarget = false; // for vive we want to allow separate render target to get the scaling done
#else
	_noOutputRenderTarget = true; // we should not use post process anyway
#endif
#else
	_noOutputRenderTarget = false; // this allows for post process
#endif

	struct SwapchainUtil
	{
		uint32_t swapchainFormatCount;
		Array<int64_t> swapchainFormats;

		bool init(OpenXRImpl* _impl, XrSession _xrSession)
		{
			{
				XrResult result = xrEnumerateSwapchainFormats(_xrSession, 0, &swapchainFormatCount, nullptr);
				if (!_impl->check_openxr(result, TXT("get number of supported swapchain formats")))
				{
					return false;
				}
			}

			swapchainFormats.set_size(swapchainFormatCount);
			{
				XrResult result = xrEnumerateSwapchainFormats(_xrSession, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.begin());
				if (!_impl->check_openxr(result, TXT("enumerate swapchain formats")))
				{
					return false;
				}
			}

			return true;
		}

		int64_t get_format(int64_t _preferredFormat, bool _getAny = true)
		{
			int64_t chosenFormat = _getAny ? swapchainFormats[0] : -1;

			for_every(sf, swapchainFormats)
			{
#ifdef OUTPUT_SWAPCHAIN_FORMATS
				output(TXT(" %X v %X"), (int)*sf, (int)_preferredFormat);
#endif
				if (*sf == _preferredFormat)
				{
#ifdef OUTPUT_SWAPCHAIN_FORMATS
					output(TXT(" choose!"));
#endif
					chosenFormat = *sf;
					break;
				}
			}

			return chosenFormat;
		}

		void output_available_formats()
		{
			output(TXT("available formats:"));
			for_every(sf, swapchainFormats)
			{
				output(TXT("  %X"), (int)*sf);
			}
		}
	};

	SwapchainUtil swapchainUtil;

	// get swap chain formats, do we use them?
	if (!swapchainUtil.init(this, xrSession))
	{
		isOk = false;
		return;
	}

	/**
	 *  IMPORTANT!
	 *	for GLES we use rendering to SRGB but WITHOUT converting to SRGB
	 *	ie. requestedColourFormat = GL_SRGB8_ALPHA8
	 *	when binding framebuffer object, glDisable(GL_FRAMEBUFFER_SRGB);
	 *	this is handled via the setup by explicitly ordering to disable SRGB rendering
	 */
	//int64_t requestedColourFormat = GL_RGBA8;
	int64_t requestedColourFormat = GL_SRGB8_ALPHA8;
	Optional<bool> systemImplicitSRGB;
	Optional<bool> forcedSRGBConversion;
	int64_t colourFormat = 0;
	int64_t depthFormat = 0;
	bool depthWithStencil = false;

	output(TXT("[system-openxr] setup output"));
#ifdef AN_GLES
	//if (system.isOculus) // for time being assume all gles have this issue
	{
		output(TXT("[system-openxr] using gles, disable srgb conversion"));
		// disallow srgb conversion
		forcedSRGBConversion = false;
		// as it is system implicit
		systemImplicitSRGB = true;
	}
#else
	//forcedSRGBConversion = true;
#endif

	// if we were to use depth swapchain, we should either use it with direct-to-vr or copy
	// copying is not yet implemented!
	bool useDepthSwapchain = false;
	bool useDepthRenderBuffer = true;

	if (!_useDirectToVRRendering)
	{
		useDepthSwapchain = false;
		useDepthRenderBuffer = false;
	}

	{
		output(TXT("[system-openxr] find colour format %X"), (int)requestedColourFormat);
#ifdef INSPECT_VIVE_COSMOS
		swapchainUtil.output_available_formats();
#endif
		colourFormat = swapchainUtil.get_format(requestedColourFormat, true);
		if (colourFormat != requestedColourFormat)
		{
			error(TXT("[system-openxr] invalid colour format"));
		}
		output(TXT("[system-openxr] colour format used %X"), (int)colourFormat);

		if (_useDirectToVRRendering)
		{
			int64_t requestedDepthFormat = GL_DEPTH24_STENCIL8;
			depthFormat = requestedDepthFormat;
			if (useDepthSwapchain)
			{
				output(TXT("[system-openxr] find depth format %X"), (int)requestedDepthFormat);
				depthFormat = swapchainUtil.get_format(requestedDepthFormat, false);
				if (depthFormat != requestedDepthFormat)
				{
					error(TXT("[system-openxr] depth format depth 24, stencil 8 not supported (req:%X, prov:%X)"), requestedDepthFormat, depthFormat);
					warn(TXT("[system-openxr] don't use depth swapchain"));
					swapchainUtil.output_available_formats();
					// disable functionality
					useDepthSwapchain = false;
				}
			}
			depthWithStencil = ::System::DepthStencilFormat::has_stencil((::System::DepthStencilFormat::Type)depthFormat);
			output(TXT("[system-openxr] depth format used %X"), (int)depthFormat);
			output(TXT("[system-openxr] depth with stencil format used %S"), depthWithStencil ? TXT("YES") : TXT("no"));
		}
	}

	if (!useSingleRenderTarget)
	{
		// we can't work with multiple buffers this way
		useSingleFrameBuffer = false;
		useSingleDepthRenderBuffer = false;
	}

#ifdef INSPECT_VIVE_COSMOS
	output(TXT("[system-openxr] %S"), useSingleRenderTarget ? TXT("use single render buffer") : TXT("using multiple render targets, one per image"));
	output(TXT("[system-openxr] %S"), useSingleFrameBuffer ? TXT("use single frame buffer") : TXT("using multiple frame buffers, one per image"));
#endif

#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
	XrFoveationLevelFB useFoveationLevel = XR_FOVEATION_LEVEL_NONE_FB;
	{
		auto vraf = get_available_functionality();
		float configLevel = MainConfig::global().get_video().vrFoveatedRenderingLevel;
		useFoveationLevel = (XrFoveationLevelFB)vraf.convert_foveated_rendering_config_to_level(configLevel);
	}
	xrFoveationUsed = false;
	if (xrFoveationAvailable)
	{
		output(TXT("[system-openxr] foveation level %i"), useFoveationLevel);
		xrFoveationUsed = useFoveationLevel > 0;
	}
#endif
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_HTC
	XrFoveationModeHTC useFoveationMode = XR_FOVEATION_MODE_DISABLE_HTC;
	{
		auto vraf = get_available_functionality();
		float configLevel = MainConfig::global().get_video().vrFoveatedRenderingLevel;
		useFoveationMode = (XrFoveationModeHTC)vraf.convert_foveated_rendering_config_to_level(configLevel);
	}
	xrFoveationUsed = false;
	if (xrFoveationAvailable)
	{
		output(TXT("[system-openxr] foveation mode %i"), useFoveationMode);
		xrFoveationUsed = useFoveationMode > 0;
	}
#endif

#ifdef OWN_FOVEATED_IMPLEMENTATION
	ownFoveationUsed = ::System::FoveatedRenderingSetup::should_use() && owner->does_use_direct_to_vr_rendering();
#endif

	output(TXT("[system-openxr] : _source has %i output(s)"), _source.get_output_texture_count());	

	for_count(int, rmIdx, InternalRenderMode::COUNT)
	{
		output(TXT("[system-openxr] rm:%i"), rmIdx);
		auto& rmd = renderModeData[rmIdx];

		bool depthInUse = false;

		Array<::System::RenderTargetSetup> useSetups;

#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
		if (xrFoveationAvailable && xrFoveationUsed && ext_xrCreateFoveationProfileFB)
		{
			XrFoveationLevelProfileCreateInfoFB levelProfileCreateInfo;
			memory_zero(levelProfileCreateInfo);
			levelProfileCreateInfo.type = XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB;
			levelProfileCreateInfo.level = min(useFoveationLevel, XR_FOVEATION_LEVEL_HIGH_FB);
			levelProfileCreateInfo.verticalOffset = 0.0f;
			levelProfileCreateInfo.dynamic = XR_FOVEATION_DYNAMIC_DISABLED_FB;
			levelProfileCreateInfo.next = nullptr;

			XrFoveationProfileCreateInfoFB profileCreateInfo;
			memory_zero(profileCreateInfo);
			profileCreateInfo.type = XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB;
			profileCreateInfo.next = &levelProfileCreateInfo;

			output(TXT("[system-openxr] create foveation profile, level %i"), levelProfileCreateInfo.level);
			check_openxr(ext_xrCreateFoveationProfileFB(xrSession, &profileCreateInfo, &rmd.xrFoveationProfile), TXT("create foveation profile"));
		}
#endif

		// create swap chain (and render targets)
		rmd.viewSize.set_size(xrConfigViews.get_size());
		rmd.aspectRatioCoef.set_size(xrConfigViews.get_size());
		useSetups.set_size(xrConfigViews.get_size());
		rmd.xrSwapchains.set_size(xrConfigViews.get_size());
		for_every(xsc, rmd.xrSwapchains)
		{
			int eyeIdx = for_everys_index(xsc);

			output(TXT("[system-openxr] : prepare use setups for eye %i"), eyeIdx);

			auto& vs = rmd.viewSize[eyeIdx];
			auto& aspectRatioCoef = rmd.aspectRatioCoef[eyeIdx];
			auto& useSetup = useSetups[eyeIdx];

			useSetup = _source;

			{
				vs = useSetup.get_size();

				if (vs.is_zero())
				{
					vs = recommendedViewSize[eyeIdx];
				}
				if (vs.is_zero())
				{
					vs = _fallbackEyeResolution * rmd.renderInfo.renderScale[eyeIdx];
				}
				float useVRResolutionCoef = MainConfig::global().get_video().vrResolutionCoef;
				if (rmIdx == InternalRenderMode::HiRes &&
					rmIdx != InternalRenderMode::Normal)
				{
					useVRResolutionCoef += 0.1f;
				}
				vs *= useVRResolutionCoef * MainConfig::global().get_video().overall_vrResolutionCoef; // gl draw buffers? || what, what do you mean "gl draw buffers"?
				aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef; // store it for further use
				vs = apply_aspect_ratio_coef(vs, aspectRatioCoef);

				useSetup.set_size(vs);
			}

			if (useSetup.get_output_texture_count() >= 1)
			{
				auto& otd = useSetup.access_output_texture_definition(0);
				otd.set_format((::System::VideoFormat::Type)colourFormat);
			}

			if (systemImplicitSRGB.is_set())
			{
				::System::Video3D::get()->set_system_implicit_srgb(systemImplicitSRGB.get());
			}

			useSetup.for_eye_idx(eyeIdx);
			useSetup.set_forced_srgb_conversion(forcedSRGBConversion);
			useSetup.set_depth_stencil_format((::System::DepthStencilFormat::Type)depthFormat);
			if (_useDirectToVRRendering)
			{
				useSetup.set_msaa_samples(MainConfig::global().get_video().get_vr_msaa_samples());
			}

			// images
			{
#ifdef INSPECT_VIVE_COSMOS
				output(TXT("[system-openxr] create swapchain colour format: %X, sample count: %i"), colourFormat, max(1, useSetup.get_msaa_samples()));
#endif

				{
					output(TXT("[system-openxr] create swapchain"));
					XrSwapchainCreateInfo swapchainCreateInfo;
					memory_zero(swapchainCreateInfo);
					swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
					swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
					swapchainCreateInfo.createFlags = 0;
					swapchainCreateInfo.format = colourFormat;
					swapchainCreateInfo.sampleCount = max(1, useSetup.get_msaa_samples());
					swapchainCreateInfo.width = vs.x;
					swapchainCreateInfo.height = vs.y;
					swapchainCreateInfo.faceCount = 1;
					swapchainCreateInfo.arraySize = 1;
					swapchainCreateInfo.mipCount = 1;
					swapchainCreateInfo.next = nullptr;

#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
					XrSwapchainCreateInfoFoveationFB swapchainFoveationCreateInfo;
					//XrSwapchainStateFoveationFB foveationUpdateState;
					if (xrFoveationAvailable && xrFoveationUsed)
					{
						output(TXT("[system-openxr] swapchain uses foveated rendering"));
						memory_zero(swapchainFoveationCreateInfo);
						swapchainFoveationCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB;
						/*
						if (::System::VideoGLExtensions::get().QCOM_texture_foveated)
						{
							output(TXT("[system-openxr] explicitly use scaled bin foveation"));
							swapchainFoveationCreateInfo.flags = XR_SWAPCHAIN_CREATE_FOVEATION_SCALED_BIN_BIT_FB; // explicitly told to do so
						}
						*/
						swapchainCreateInfo.next = &swapchainFoveationCreateInfo;

						/*
						// add info about profile
						swapchainFoveationCreateInfo.next = &foveationUpdateState;
						memory_zero(foveationUpdateState);
						foveationUpdateState.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB;
						foveationUpdateState.profile = rmd.xrFoveationProfile;
						*/
					}
#endif

					XrResult result = xrCreateSwapchain(xrSession, &swapchainCreateInfo, &xsc->xrSwapchain);
					if (!check_openxr(result, TXT("create swapchain %d"), for_everys_index(xsc)))
					{
						isOk = false;
						return;
					}
				}

				uint32_t imageCount = 0;

				{
					XrResult result = xrEnumerateSwapchainImages(xsc->xrSwapchain, 0, &imageCount, nullptr);
					if (!check_openxr(result, TXT("enumerate swapchains")))
					{
						isOk = false;
						return;
					}
				}

				output(TXT("[system-openxr] image count: %i"), imageCount);

				xsc->images.set_size(imageCount);
				xsc->foveatedSetupCount = 0;
				for_every(img, xsc->images)
				{
					memory_zero(*img);
#ifdef AN_GL
					img->type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
#endif
#ifdef AN_GLES
					img->type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#endif
					img->next = nullptr;
				}

				{
					XrResult result = xrEnumerateSwapchainImages(xsc->xrSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)xsc->images.begin());
					if (!check_openxr(result, TXT("enumerate swapchain images")))
					{
						isOk = false;
						return;
					}
				}

				{
					for_every(img, xsc->images)
					{
						/*
						DIRECT_GL_CALL_ glBindTexture(GL_TEXTURE_2D, img->image); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glBindTexture(GL_TEXTURE_2D, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
						*/

						// do we need to prepare them? maybe we should use them as they are? filtering/wrapping etc
						//::System::RenderTargetUtils::prepare_render_target_texture(img->image); // if we call it, we get errors
					}
				}

#ifdef SETUP_FOVEATION_RIGHT_AFTER_CREATED_SWAPCHAIN
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
				if (ext_xrUpdateSwapchainFB
					&& rmd.xrFoveationProfile != XR_NULL_HANDLE)
				{
					output(TXT("[system-openxr] update swapchain with foveated profile"));
					XrSwapchainStateFoveationFB foveationUpdateState;
					memory_zero(foveationUpdateState);
					foveationUpdateState.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB;
					foveationUpdateState.profile = rmd.xrFoveationProfile;

					check_openxr(ext_xrUpdateSwapchainFB(xsc->xrSwapchain, (XrSwapchainStateBaseHeaderFB*)(&foveationUpdateState)), TXT("set foveation via update swapchain"));
				}
#endif
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_HTC
				if (ext_xrApplyFoveationHTC)
				{
					output(TXT("[system-openxr] apply foveation"));

					XrSwapchainSubImage xrSubImage;
					xrSubImage.swapchain = xsc->xrSwapchain;
					xrSubImage.imageRect.offset.x = 0;
					xrSubImage.imageRect.offset.y = 0;
					xrSubImage.imageRect.extent.width = vs.x;
					xrSubImage.imageRect.extent.height = vs.y;
					xrSubImage.imageArrayIndex = 0;

					// !@#
					//useFoveationMode = XR_FOVEATION_MODE_DISABLE_HTC;
					useFoveationMode = XR_FOVEATION_MODE_FIXED_HTC;
					//useFoveationMode = XR_FOVEATION_MODE_DYNAMIC_HTC;

					XrFoveationApplyInfoHTC foveationApplyInfo;
					memory_zero(foveationApplyInfo);
					foveationApplyInfo.type = XR_TYPE_FOVEATION_APPLY_INFO_HTC;
					foveationApplyInfo.mode = useFoveationMode;
					foveationApplyInfo.subImageCount = 1;
					foveationApplyInfo.subImages = &xrSubImage;

					REMOVE_OR_CHANGE_BEFORE_COMMIT_
					output(TXT("[system-openxr] useFoveationMode:%i"), useFoveationMode);

					XrFoveationDynamicModeInfoHTC dynamicFoveation;
					if (useFoveationMode == XR_FOVEATION_MODE_DYNAMIC_HTC)
					{
						REMOVE_OR_CHANGE_BEFORE_COMMIT_
						output(TXT("[system-openxr] dynamic"));
						memory_zero(dynamicFoveation);
						dynamicFoveation.type = XR_TYPE_FOVEATION_DYNAMIC_MODE_INFO_HTC;
						// this should allow the system to adjust everything (based on eye?)
						dynamicFoveation.dynamicFlags = XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_BIT_HTC
													  | XR_FOVEATION_DYNAMIC_CLEAR_FOV_ENABLED_BIT_HTC
													  | XR_FOVEATION_DYNAMIC_FOCAL_CENTER_OFFSET_ENABLED_BIT_HTC;
						foveationApplyInfo.next = &dynamicFoveation;
					}

					check_openxr(ext_xrApplyFoveationHTC(xrSession, &foveationApplyInfo), TXT("set foveation via apply foveation"));
				}
#endif
#endif
			}

			// create depth if supported (we may use it to provide depth data to the system for time warping)
			if (useDepthSwapchain)
			{
#ifdef INSPECT_VIVE_COSMOS
				output(TXT("[system-openxr] create depth swapchain"));
#endif

				depthInUse = true;
				xsc->withDepthSwapchain = true;
				xsc->withDepthStencil = depthWithStencil;

				{
					XrSwapchainCreateInfo swapchainCreateInfo;
					memory_zero(swapchainCreateInfo);
					swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
					swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					swapchainCreateInfo.createFlags = 0;
					swapchainCreateInfo.format = depthFormat;
					swapchainCreateInfo.sampleCount = max(1, useSetup.get_msaa_samples());
					swapchainCreateInfo.width = vs.x;
					swapchainCreateInfo.height = vs.y;
					swapchainCreateInfo.faceCount = 1;
					swapchainCreateInfo.arraySize = 1;
					swapchainCreateInfo.mipCount = 1;
					swapchainCreateInfo.next = nullptr;

					XrResult result = xrCreateSwapchain(xrSession, &swapchainCreateInfo, &xsc->xrDepthSwapchain);
					if (!check_openxr(result, TXT("create swapchain %d"), for_everys_index(xsc)))
					{
						isOk = false;
						return;
					}
				}

				uint32_t imageCount = 0;

				{
					XrResult result = xrEnumerateSwapchainImages(xsc->xrDepthSwapchain, 0, &imageCount, nullptr);
					if (!check_openxr(result, TXT("enumerate swapchains")))
					{
						isOk = false;
						return;
					}
				}

				xsc->depthImages.set_size(imageCount);
				for_every(img, xsc->depthImages)
				{
					memory_zero(*img);
#ifdef AN_GL
					img->type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
#endif
#ifdef AN_GLES
					img->type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#endif
					img->next = nullptr;
				}

				{
					XrResult result = xrEnumerateSwapchainImages(xsc->xrDepthSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)xsc->depthImages.begin());
					if (!check_openxr(result, TXT("enumerate swapchain images")))
					{
						isOk = false;
						return;
					}
				}

#ifdef SETUP_FOVEATION_RIGHT_AFTER_CREATED_SWAPCHAIN
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
				if (ext_xrUpdateSwapchainFB && rmd.xrFoveationProfile != XR_NULL_HANDLE)
				{
#ifdef ELABORATE_DEBUG_LOGGING
					output(TXT("[system-openxr] update swapchain with foveated profile (depth chain)"));
#endif
					XrSwapchainStateFoveationFB foveationUpdateState;
					memory_zero(foveationUpdateState);
					foveationUpdateState.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB;
					foveationUpdateState.profile = rmd.xrFoveationProfile;

					check_openxr(ext_xrUpdateSwapchainFB(xsc->xrDepthSwapchain, (XrSwapchainStateBaseHeaderFB*)(&foveationUpdateState)), TXT("set foveation via update swapchain"));
				}
#endif
#endif
			}

			output(TXT("[system-openxr] created images for swapchain #%i: %i + %i"), for_everys_index(xsc), xsc->images.get_size(), xsc->depthImages.get_size());

			an_assert(xsc->depthImages.is_empty() || xsc->images.get_size() == xsc->depthImages.get_size(), TXT("if they do not match, but we're just checking"));

			// setup render buffers and textures
			{

				// arrange space to hold render buffers and depth render buffers
				{
					xsc->imageFrameBufferIDs.set_size(useSingleFrameBuffer ? 1 : xsc->images.get_size());
					if (useDepthRenderBuffer)
					{
						depthInUse = true;
						xsc->withDepthRenderBuffer = true;
						xsc->withDepthStencil = depthWithStencil;

						xsc->depthRenderBufferIDs.set_size(useSingleDepthRenderBuffer ? 1 : xsc->images.get_size());
					}
				}

				// prepare textures and render buffers
				for_every(img, xsc->images)
				{
#ifdef INSPECT_VIVE_COSMOS
					output(TXT("[system-openxr] for image idx %i use colour texture provided by openxr [%i]"), for_everys_index(img), img->image);
#endif

					const GLuint colourTexture = img->image;

					auto& glExtensions = ::System::VideoGLExtensions::get();

#ifdef INSPECT_VIVE_COSMOS
					output(TXT("[system-openxr] setup texture [%i]"), colourTexture);
#endif

					// texture
					{
						GLenum colourTextureTarget = GL_TEXTURE_2D;
						DIRECT_GL_CALL_ glBindTexture(colourTextureTarget, colourTexture); AN_GL_CHECK_FOR_ERROR_ALWAYS

#ifdef AN_GLES
						if (glExtensions.EXT_texture_border_clamp)
						{
							DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR_ALWAYS
							DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR_ALWAYS
							GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
							DIRECT_GL_CALL_ glTexParameterfv(colourTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor); AN_GL_CHECK_FOR_ERROR_ALWAYS
						}
						else
						{
							// Just clamp to edge. However, this requires manually clearing the border
							// around the layer to clear the edge texels.
							DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR_ALWAYS
							DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR_ALWAYS
						}
#else
						{
							DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR_ALWAYS
							DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR_ALWAYS
							GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
							DIRECT_GL_CALL_ glTexParameterfv(colourTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor); AN_GL_CHECK_FOR_ERROR_ALWAYS
						}
#endif

						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glBindTexture(colourTextureTarget, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
					}

					auto& imageFrameBufferID = xsc->imageFrameBufferIDs[for_everys_index(img)];
					GLenum frameBufferTarget = useSetup.should_use_msaa() ? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
#ifdef INSPECT_VIVE_COSMOS
					output(TXT("[system-openxr] frame buffer target %X (%S), because of msaa samples (%S)"), frameBufferTarget, frameBufferTarget == GL_FRAMEBUFFER? TXT("GL_FRAMEBUFFER") : TXT("GL_DRAW_FRAMEBUFFER"), useSetup.should_use_msaa()? TXT("use msaa") : TXT("no msaa"));
#endif

					// frame buffer
					{
						// create the frame buffer
						DIRECT_GL_CALL_ glGenFramebuffers(1, &imageFrameBufferID); AN_GL_CHECK_FOR_ERROR_ALWAYS
#ifdef INSPECT_VIVE_COSMOS
						output(TXT("[system-openxr] created image frame buffer [%i], attaching colour texture [%i], msaa samples: %i"), imageFrameBufferID, colourTexture, useSetup.get_msaa_samples());
#endif
						::System::RenderTargetUtils::attach_colour_texture_to_frame_buffer(imageFrameBufferID, colourTexture, useSetup.get_msaa_samples());
					}

					// create and attach depth buffer (only for direct to vr rendering)
					if (useDepthRenderBuffer)
					{
#ifdef INSPECT_VIVE_COSMOS
						output(TXT("[system-openxr] use depth (%S) render buffer"), xsc->withDepthStencil? TXT("with depth stencil") : TXT("no depth stencil"));
#endif
						auto& depthRenderBufferID = xsc->depthRenderBufferIDs[for_everys_index(img)];
						DIRECT_GL_CALL_ glGenRenderbuffers(1, &depthRenderBufferID); AN_GL_CHECK_FOR_ERROR_ALWAYS

						::System::RenderTargetUtils::prepare_depth_stencil_render_buffer(depthRenderBufferID,
							::System::RenderTargetUtils::PrepareDepthStencilRenderBufferParams()
							.with_size(vs)
							.with_stencil(xsc->withDepthStencil)
							.with_msaa_samples(useSetup.get_msaa_samples())
						);

#ifdef INSPECT_VIVE_COSMOS
						output(TXT("[system-openxr] attach depth render buffer [%i] to image frame buffer [%i]"), depthRenderBufferID, imageFrameBufferID);
#endif

						// attach depth render buffer to image frame buffer
						::System::RenderTargetUtils::attach_depth_stencil_render_buffer_to_frame_buffer(imageFrameBufferID, depthRenderBufferID, xsc->withDepthStencil);
					}

					// check for errors
					{
#ifdef INSPECT_VIVE_COSMOS
						output(TXT("[system-openxr] check status target %X, image frame buffer [%i]"), frameBufferTarget, imageFrameBufferID);
#endif

						DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, imageFrameBufferID); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ GLenum renderFramebufferStatus = glCheckFramebufferStatus(frameBufferTarget); AN_GL_CHECK_FOR_ERROR_ALWAYS
						DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS

						if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
						{
							if (renderFramebufferStatus == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
							{
								error(TXT("[system-openxr] render frame buffer incomplete - incomplete attachment"));
							}
							else if (renderFramebufferStatus == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
							{
								error(TXT("[system-openxr] render frame buffer incomplete - missing attachment"));
							}
#ifndef AN_GLES
							else if (renderFramebufferStatus == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
							{
								error(TXT("[system-openxr] render frame buffer incomplete - incomplete draw buffer"));
							}
							else if (renderFramebufferStatus == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
							{
								error(TXT("[system-openxr] render frame buffer incomplete - incomplete read buffer"));
							}
#endif
							else if (renderFramebufferStatus == GL_FRAMEBUFFER_UNSUPPORTED)
							{
								error(TXT("[system-openxr] render frame buffer incomplete - frame buffer unsupported"));
							}
							else
							{
								error(TXT("[system-openxr] render frame buffer incomplete"));
							}
							return;
						}
					}
				}
			}

			output(TXT("[system-openxr] : prepared textures and render buffers"));

			if (_useDirectToVRRendering)
			{
				output(TXT("[system-openxr] : using direct to vr rendering, prepare proper render target setup proxies"));
				an_assert(useSingleRenderTarget || (!useSingleFrameBuffer && (!useDepthRenderBuffer || !useSingleDepthRenderBuffer)));
				xsc->directToVRRenderTargets.set_size(useSingleRenderTarget ? 1 : xsc->images.get_size());
				for_every(directToVRRenderTarget, xsc->directToVRRenderTargets)
				{
					int idx = for_everys_index(directToVRRenderTarget);
					System::RenderTargetSetupDynamicProxy dynamicProxySetup(useSetup);
					System::RenderTargetSetupProxy staticProxySetup(useSetup);
					{
						System::RenderTargetSetupProxy* proxySetup = useSingleRenderTarget ? &dynamicProxySetup : &staticProxySetup;
						proxySetup->colourTexture = xsc->images[idx].image;
						proxySetup->frameBufferObject = xsc->imageFrameBufferIDs[idx];
						if (xsc->withDepthSwapchain)
						{
							proxySetup->depthStencilTexture = xsc->depthImages[idx].image;
						}
						if (xsc->withDepthRenderBuffer)
						{
							proxySetup->depthStencilBufferObject = xsc->depthRenderBufferIDs[idx];
						}
					}
					if (useSingleRenderTarget)
					{
						output(TXT("[system-openxr] : single render target using dynamic proxy setup"));
						RENDER_TARGET_CREATE_INFO(TXT("openxr, direct to vr, single render target, rm %i, eye %i"), rmIdx, eyeIdx);
						*directToVRRenderTarget = new ::System::RenderTarget(dynamicProxySetup);
					}
					else
					{
						output(TXT("[system-openxr] : static proxy setup #%i"), for_everys_index(directToVRRenderTarget));
						RENDER_TARGET_CREATE_INFO(TXT("openxr, direct to vr, many render targets, rm %i, eye %i, rt %i"), rmIdx, eyeIdx, idx);
						*directToVRRenderTarget = new ::System::RenderTarget(staticProxySetup);
					}
				}
#ifdef OWN_FOVEATED_IMPLEMENTATION
				for_every(img, xsc->images)
				{
					if (ownFoveationUsed)
					{
						::System::FoveatedRenderingSetup frs;
						frs.setup((Eye::Type)eyeIdx, rmd.renderInfo.lensCentreOffset[eyeIdx], vs.to_vector2(), 1.0f);
						::System::RenderTargetUtils::setup_foveated_rendering_for_texture(eyeIdx, img->image, frs);
					}
				}
#endif
			}
		}

		// create xr projection views
		{
			output(TXT("[system-openxr] : create xr projectioni views"));
			rmd.xrProjectionViews.set_size(xrConfigViews.get_size());
			for_every(xpv, rmd.xrProjectionViews)
			{
				int i = for_everys_index(xpv);
				auto& vs = rmd.viewSize[i];

				memory_zero(*xpv);
				xpv->type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
				xpv->next = nullptr;

				xpv->subImage.swapchain = rmd.xrSwapchains[i].xrSwapchain;
				xpv->subImage.imageArrayIndex = 0;
				xpv->subImage.imageRect.offset.x = 0;
				xpv->subImage.imageRect.offset.y = 0;
				xpv->subImage.imageRect.extent.width = vs.x;
				xpv->subImage.imageRect.extent.height = vs.y;

				// pose and fov will be updated every frame
			}
		}

		// create xr depth infos
		if (depthInUse && useDepthSwapchain)
		{
			output(TXT("[system-openxr] : create depth infos"));
			rmd.xrDepthInfos.set_size(xrConfigViews.get_size());
			for_every(xdi, rmd.xrDepthInfos)
			{
				int i = for_everys_index(xdi);
				auto& vs = rmd.viewSize[i];

				memory_zero(*xdi);
				xdi->type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
				xdi->next = nullptr;

				xdi->minDepth = 0.0f;
				xdi->maxDepth = 1.0f;
				xdi->nearZ = 0.1f;
				xdi->farZ = 10000.0f;

				xdi->subImage.swapchain = rmd.xrSwapchains[i].xrDepthSwapchain;
				xdi->subImage.imageArrayIndex = 0;
				xdi->subImage.imageRect.offset.x = 0;
				xdi->subImage.imageRect.offset.y = 0;
				xdi->subImage.imageRect.extent.width = vs.x;
				xdi->subImage.imageRect.extent.height = vs.y;

				// depth is chained to projection, not submitted as separate layer
				rmd.xrProjectionViews[i].next = xdi;

				// minDepth, maxDepth, nearZ, farZ updated as required?
			};
		}
	}

	output(TXT("[system-openxr] : all internal render targets done"));
}

VectorInt2 OpenXRImpl::get_direct_to_vr_render_size(int _eyeIdx)
{
	auto& rmd = renderModeData[renderMode];
	if (rmd.viewSize.is_index_valid(_eyeIdx))
	{
		return rmd.viewSize[_eyeIdx];
	}
	return VectorInt2::zero;
}

float OpenXRImpl::get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx)
{
	auto& rmd = renderModeData[renderMode];
	if (rmd.aspectRatioCoef.is_index_valid(_eyeIdx))
	{
		return rmd.aspectRatioCoef[_eyeIdx];
	}
	return 1.0f;
}

::System::RenderTarget* OpenXRImpl::get_direct_to_vr_render_target(int _eyeIdx)
{
	auto& rmd = renderModeData[renderMode];
	if (begunFrame && _eyeIdx < Eye::Count && _eyeIdx < rmd.xrSwapchains.get_size())
	{
		auto& xrSwapchain = rmd.xrSwapchains[_eyeIdx];
		auto& brfe = rmd.begunRenderForEye[_eyeIdx];
		bool acquiredNow = false;
		if (!brfe.acquiredImageIndex.is_set())
		{
			begin_render_for_eye(::System::Video3D::get(), (Eye::Type)_eyeIdx);
			acquiredNow = true;
		}

		if (brfe.acquiredImageIndex.is_set())
		{
			int imageIdx = brfe.acquiredImageIndex.get();

			if (auto* rt = xrSwapchain.directToVRRenderTargets[useSingleRenderTarget? 0 : imageIdx].get())
			{
				if (rt->is_bound())
				{
					// we must have bound it already
					return rt;
				}

				// setup dynamic proxy
				if (useSingleRenderTarget)
				{
					::System::RenderTargetProxyData rtProxyData;
					rtProxyData.colourTexture = xrSwapchain.images[imageIdx].image;
					rtProxyData.frameBufferObject = xrSwapchain.imageFrameBufferIDs[useSingleFrameBuffer? 0 : imageIdx];
					if (xrSwapchain.withDepthSwapchain && brfe.acquiredDepthImageIndex.is_set())
					{
						rtProxyData.depthStencilTexture = xrSwapchain.depthImages[brfe.acquiredDepthImageIndex.get()].image;
					}
					if (xrSwapchain.withDepthRenderBuffer)
					{
						rtProxyData.depthStencilBufferObject = xrSwapchain.depthRenderBufferIDs[useSingleDepthRenderBuffer? 0 : imageIdx];
					}
					rt->set_data_for_dynamic_proxy(rtProxyData);
				}

#ifdef OWN_FOVEATED_IMPLEMENTATION
				if (acquiredNow && ownFoveationUsed)
				{
					if (::System::FoveatedRenderingSetup::should_force_set_foveation())
					{
						xrSwapchain.foveatedSetupCount = 0;
					}

					todo_hack(TXT("to make sure it works"));
					if (xrSwapchain.foveatedSetupCount < xrSwapchain.images.get_size() * 3)
					{
						++xrSwapchain.foveatedSetupCount;
						::System::FoveatedRenderingSetup frs;
						frs.setup((Eye::Type)_eyeIdx, rmd.renderInfo.lensCentreOffset[_eyeIdx], rmd.viewSize[_eyeIdx].to_vector2(), 1.0f);
						::System::RenderTargetUtils::setup_foveated_rendering_for_texture(_eyeIdx, xrSwapchain.images[imageIdx].image, frs);
					}
				}
#endif

				return rt;
			}
		}
	}
	return nullptr;
}

void OpenXRImpl::on_render_targets_created()
{
}

void OpenXRImpl::next_frame()
{
	if (!is_ok())
	{
#ifdef AN_LOG_FRAME_CALLS
		output(TXT("[system-openxr] : next frame FAILED"));
#endif
		return;
	}

#ifdef AN_LOG_FRAME_CALLS
	{
		static int xrNextFrame = 0;
		output(TXT("[system-openxr] : next frame %i"), xrNextFrame);
		++xrNextFrame;
	}
#endif
}

void OpenXRImpl::game_sync_impl()
{
	if (!xrSessionRunning)
	{
		xrFrameSessionShouldRender = false;
		return;
	}

	/*
	if (xrSessionVisible)
	{
		// go straight to test loop to see what we can get! timing-wise
		test_loop();
	}
	*/

	XrFrameState frameState;
	memory_zero(frameState);
	frameState.type = XR_TYPE_FRAME_STATE;
	frameState.next = nullptr;
	XrFrameWaitInfo frameWaitInfo;
	memory_zero(frameWaitInfo);
	frameWaitInfo.type = XR_TYPE_FRAME_WAIT_INFO;
	frameWaitInfo.next = nullptr;

#ifdef AN_LOG_FRAME_CALLS
	{
		static int waitFrame = 0;
		output(TXT("[system-openxr] : xrWaitFrame %i"), waitFrame);
		++waitFrame;
	}
#endif

	{
		MEASURE_PERFORMANCE_COLOURED(xrWaitFrame, Colour::black);

#ifdef USE_EXTRA_WAIT_FOR_GAME_SYNC
#ifdef AN_LINUX_OR_ANDROID
		Optional<float> useDisplayRefreshRate;
		useDisplayRefreshRate.set_if_not_set(displayRefreshRate);
		useDisplayRefreshRate.set_if_not_set(displayRefreshRateFromWaitFrame);
		if (useDisplayRefreshRate.is_set())
		{
			float frameTime = 1.0f / useDisplayRefreshRate.get();
			float timeReservedUS = 1000.0f;
			float timeLeft = frameTime - gameSyncTS.get_time_since();
			float timeLeftUS = timeLeft * 1000000.0f;
			timeLeftUS -= timeReservedUS;
			if (timeLeftUS > 0.0f)
			{
				MEASURE_PERFORMANCE_COLOURED(extraSleep, Colour::greyLight);
				System::Core::sleep_u(timeLeftUS);
			}
		}
#endif
#endif

#ifdef OUTPUT_ALL_LONG_WAITS
		::System::TimeStamp waitFrameTS;
#endif
		XrResult result = xrWaitFrame(xrSession, &frameWaitInfo, &frameState);
#ifdef AN_LOG_FRAME_CALLS
		output(TXT("OpenXRImpl : waitFrame result: %i"), result);
#endif
		if (!check_openxr(result, TXT("wait frame")))
		{
			return;
		}

#ifdef OUTPUT_ALL_LONG_WAITS
		{
			float waitFrameTime = waitFrameTS.get_time_since();
			float idealExpectedFrameTime = calculate_ideal_expected_frame_time();
#ifndef OUTPUT_ALL_LONG_WAITS_ALL
			if (waitFrameTime > idealExpectedFrameTime)
#endif
			{
				static int framesHitched = 0;
				++framesHitched;
				output(TXT("[system-openxr-hitch-frame] wait frame (%i) %.3fms > %.3fms"), framesHitched, waitFrameTime * 1000.0f, idealExpectedFrameTime * 1000.0f);
			}
		}
#endif

		gameSyncTS.reset();
		// marker at end
		PERFORMANCE_MARKER(Colour::black);
	}

	{
		syncPredictedDisplayTime = frameState.predictedDisplayTime;
		XrTime vrSyncDeltaTime = syncPredictedDisplayTime - prevSyncPredictedDisplayTime.get(syncPredictedDisplayTime);
		prevSyncPredictedDisplayTime = syncPredictedDisplayTime;

		{
			if (frameState.predictedDisplayPeriod != XR_INFINITE_DURATION &&
				frameState.predictedDisplayPeriod != 0)
			{
				displayRefreshRateFromWaitFrame = 1.0f / (float)((double)frameState.predictedDisplayPeriod / 1000000000.0);
			}
		}

		deltaTimeFromSync = (float)((double)vrSyncDeltaTime / 1000000000.0);
	}

	if (MainConfig::global().get_vr_mode() == VRMode::PhaseSync)
	{
		// if not in phase sync, use normal approach - length of the frame
		::System::Core::force_vr_sync_delta_time(deltaTimeFromSync);
	}

	xrFrameSessionShouldRender = frameState.shouldRender;

	todo_note(TXT("maybe we should always render, the code here might be a bit bugged and might be skipping updating/sending frames, even if empty"));
	xrFrameSessionShouldRender = true;

#ifdef OUTPUT_ALL_LONG_WAITS_ALL
	output(TXT("[system-openxr-hitch-frame] begin frame gate allow one"));
#endif
		
	// we may now allow xrBeginFrame
	beginFrameGate.allow_one();
}

bool OpenXRImpl::begin_render(System::Video3D* _v3d)
{
	if (!xrSessionRunning)
	{
		return false;
	}

	if (renderMode != pendingRenderMode)
	{
		renderMode = pendingRenderMode; // switch render modes to render in a different resolution
	}

	{
		auto& rmd = renderModeData[renderMode];
		rmd.begunRenderForEye[Eye::Left].reset();
		rmd.begunRenderForEye[Eye::Right].reset();
	}

	{

#ifdef USE_EXTRA_WAIT_FOR_BEGIN_RENDER
#ifdef AN_LINUX_OR_ANDROID
		if (beginFrameSemaphore.should_wait())
		{
			Optional<float> useDisplayRefreshRate;
			useDisplayRefreshRate.set_if_not_set(displayRefreshRate);
			useDisplayRefreshRate.set_if_not_set(displayRefreshRateFromWaitFrame);
			if (useDisplayRefreshRate.is_set())
			{
				float frameTime = 1.0f / useDisplayRefreshRate.get();
				float timeReservedUS = 100.0f; // hog CPU as little as possible
				float timeSinceGS = gameSyncTS.get_time_since();
				if (timeSinceGS > frameTime * 0.1f) // in case we just dropped right after game sync updated this
				{
					float timeLeft = frameTime - timeSinceGS;
					float timeLeftUS = timeLeft * 1000000.0f;
					timeLeftUS -= timeReservedUS;
					if (timeLeftUS > 0.0f)
					{
						MEASURE_PERFORMANCE_COLOURED(extraSleep, Colour::greyLight);
						System::Core::sleep_u(timeLeftUS);
					}
				}
			}
		}
#endif
#endif

		{
			MEASURE_PERFORMANCE_COLOURED(beginFrameGate, Colour::black);
#ifdef OUTPUT_ALL_LONG_WAITS
			::System::TimeStamp beginFrameGateTS;
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (!beginFrameGate.wait_for(1.0f))
			{
				error(TXT("[system-openxr] waiting too long for frame to begin, maybe game_sync was not called?"));
			}
#else
			beginFrameGate.wait();
#endif
#ifdef OUTPUT_ALL_LONG_WAITS
			float beginFrameGateTime = beginFrameGateTS.get_time_since();
			float thresholdTime = 0.002f;
#ifndef OUTPUT_ALL_LONG_WAITS_ALL
			if (beginFrameGateTime > thresholdTime)
#endif
			{
				output(TXT("[system-openxr-hitch-frame] begin frame gate %.3fms > %.3fms"), beginFrameGateTime * 1000.0f, thresholdTime * 1000.0f);
			}
#endif
		}
#ifdef AN_LOG_FRAME_CALLS
		{
			static int beginFrame = 0;
			output_gl(TXT("OpenXRImpl : xrBeginFrame %i"), beginFrame);
			++beginFrame;
		}
#endif

		XrFrameBeginInfo frameBeginInfo;
		memory_zero(frameBeginInfo);
		frameBeginInfo.type = XR_TYPE_FRAME_BEGIN_INFO;
		frameBeginInfo.next = nullptr;

		PERFORMANCE_MARKER(Colour::green);
		MEASURE_PERFORMANCE_COLOURED(xrBeginFrame, Colour::black);

#ifdef OUTPUT_ALL_LONG_WAITS
		::System::TimeStamp beginRenderTS;
#endif
		XrResult result = xrBeginFrame(xrSession, &frameBeginInfo);
		check_openxr(result, TXT("begin frame"));
#ifdef OUTPUT_ALL_LONG_WAITS
		float beginRenderTime = beginRenderTS.get_time_since();
		float idealExpectedFrameTime = calculate_ideal_expected_frame_time();
#ifndef OUTPUT_ALL_LONG_WAITS_ALL
		if (beginRenderTime > idealExpectedFrameTime)
#endif
		{
			static int framesHitched = 0;
			++framesHitched;
			output(TXT("[system-openxr-hitch-frame] begin render (%i) %.3fms > %.3fms"), framesHitched, beginRenderTime * 1000.0f, idealExpectedFrameTime * 1000.0f);
		}
#endif
	}

	// use from sync
	predictedDisplayTime = syncPredictedDisplayTime;

	if (owner->get_render_mode() < RenderMode::HiRes &&
		MainConfig::global().get_video().allowAutoAdjustForVR &&
		owner->may_use_render_scaling())
	{
		performanceStats.frameDropsBased.periodLength = 2.0f;

		float timeSinceLastBeginFrame = beginFrameTS.get_time_since();
		beginFrameTS.reset();
		float idealExpectedFrameTime = calculate_ideal_expected_frame_time();
		float threshold = idealExpectedFrameTime * 1.25f;
		bool hitchNow = timeSinceLastBeginFrame > threshold;
		if (hitchNow)
		{
			++performanceStats.frameDropsBased.frameDropsThisPeriod;
		}
		float useThisPeriodPt = performanceStats.frameDropsBased.thisPeriodTS.get_time_since() / performanceStats.frameDropsBased.periodLength;
		float frameDrops = lerp(clamp(useThisPeriodPt, 0.0f, 1.0f), (float)performanceStats.frameDropsBased.frameDropsPrevPeriod, (float)performanceStats.frameDropsBased.frameDropsThisPeriod);
		float acceptableFrameDrops = 2.0f;
		performanceStats.frameDropsBased.fineDynamicScaleRenderTarget = max(0.1f, 1.0f - 0.03f * (max(0.0f, frameDrops - acceptableFrameDrops) / performanceStats.frameDropsBased.periodLength));
		//
		if (useThisPeriodPt >= 1.0f)
		{
#ifndef BUILD_NON_RELEASE
			if (frameDrops > acceptableFrameDrops)
#endif
			if (performanceStats.frameDropsBased.frameDropsThisPeriod > 0)
			{
				output(TXT("[system-openxr-hitch-period] %i > %i -> %.1f (%.1f/s) ->  %.3f (%.3fms > %.3fms (ideal: %.3fms)"), performanceStats.frameDropsBased.frameDropsPrevPeriod, performanceStats.frameDropsBased.frameDropsThisPeriod,
					frameDrops, frameDrops / performanceStats.frameDropsBased.periodLength,
					performanceStats.frameDropsBased.fineDynamicScaleRenderTarget,
					timeSinceLastBeginFrame * 1000.0f, threshold * 1000.0f, idealExpectedFrameTime * 1000.0f);
			}
			performanceStats.frameDropsBased.frameDropsPrevPeriod = performanceStats.frameDropsBased.frameDropsThisPeriod;
			performanceStats.frameDropsBased.frameDropsThisPeriod = 0;
			performanceStats.frameDropsBased.thisPeriodTS.reset();
		}
	}
	else
	{
		performanceStats.frameDropsBased.frameDropsPrevPeriod = 0;
		performanceStats.frameDropsBased.frameDropsThisPeriod = 0;
		performanceStats.frameDropsBased.thisPeriodTS.reset();
		performanceStats.frameDropsBased.fineDynamicScaleRenderTarget = 1.0f;
	}

	// update views only if session is in focus or visible (otherwise it is not required and we may get frozen in xrLocateViews)
	ArrayStatic<Transform, 2> readEyes;

	if (xrSessionInFocus || xrSessionVisible)
	{
		MEASURE_PERFORMANCE_COLOURED(updateViews, Colour::blue);

		XrViewLocateInfo viewLocateInfo;
		memory_zero(viewLocateInfo);
		viewLocateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
		viewLocateInfo.next = nullptr;
		viewLocateInfo.viewConfigurationType = viewType;
		viewLocateInfo.displayTime = predictedDisplayTime;
		viewLocateInfo.space = xrPlaySpace;

		XrViewState viewState;
		memory_zero(viewState);
		viewState.type = XR_TYPE_VIEW_STATE;
		viewState.next = nullptr;

		uint32_t viewCount = xrViews.get_size();
		XrResult result = xrLocateViews(xrSession, &viewLocateInfo, &viewState, viewCount, &viewCount, xrViews.begin());
		if (check_openxr(result, TXT("get view info")))
		{
			if (xrViews.get_size() >= 2)
			{
				readEyes.set_size(2);
				readEyes[0] = get_transform_from(xrViews[0].pose);
				readEyes[1] = get_transform_from(xrViews[1].pose);
			}

			for_every(xv, xrViews)
			{
				int eyeIdx = for_everys_index(xv);

				if (eyeIdx >= Eye::Count)
				{
					break;
				}
				auto& xrView = xrViews[eyeIdx];

				for_count(int, rmIdx, InternalRenderMode::COUNT)
				{
					auto& rmd = renderModeData[rmIdx];
					// read from the view
					rmd.renderInfo.fovPort[eyeIdx].left = radian_to_degree(xrView.fov.angleLeft);
					rmd.renderInfo.fovPort[eyeIdx].right = radian_to_degree(xrView.fov.angleRight);
					rmd.renderInfo.fovPort[eyeIdx].up = radian_to_degree(xrView.fov.angleUp);
					rmd.renderInfo.fovPort[eyeIdx].down = radian_to_degree(xrView.fov.angleDown);

					// store render info
					rmd.renderInfo.fov[eyeIdx] = rmd.renderInfo.fovPort[eyeIdx].get_fov();
					rmd.renderInfo.lensCentreOffset[eyeIdx] = rmd.renderInfo.fovPort[eyeIdx].get_lens_centre_offset();
				}
			}
		}
	}

	{
		MEASURE_PERFORMANCE_COLOURED(updateFoveatedRendering, Colour::blue);
		owner->update_foveated_rendering_for_render_targets();
	}

	{
		MEASURE_PERFORMANCE_COLOURED(readTrackingState, Colour::red);
		TrackingState trackingState;
		{
			MEASURE_PERFORMANCE_COLOURED(read, Colour::magenta);
			read_tracking_state(OUT_ trackingState, predictedDisplayTime);
		}
		// update offset as we have
		if (! readEyes.is_empty() && trackingState.viewPose.is_set())
		{
			Transform view = trackingState.viewPose.get();

			for_every(readEye, readEyes)
			{
				int eyeIdx = for_everys_index(readEye);

				if (eyeIdx >= Eye::Count)
				{
					break;
				}

				for_count(int, rmIdx, InternalRenderMode::COUNT)
				{
					auto& rmd = renderModeData[rmIdx];
					rmd.renderInfo.eyeOffset[eyeIdx] = view.to_local(*readEye);
				}
			}

			viewInReadViewPoseSpace = Transform::lerp(0.5f, renderModeData[0].renderInfo.eyeOffset[0], renderModeData[0].renderInfo.eyeOffset[1]);

			for (int eyeIdx = 0; eyeIdx < 2; ++eyeIdx)
			{
				for_count(int, rmIdx, InternalRenderMode::COUNT)
				{
					auto& rmd = renderModeData[rmIdx];
					rmd.renderInfo.eyeOffset[eyeIdx] = viewInReadViewPoseSpace.to_local(rmd.renderInfo.eyeOffset[eyeIdx]);
				}
			}

			if (renderModeData[InternalRenderMode::Normal].renderInfo.eyeOffset[Eye::Left].get_translation().x > 0.0f)
			{
				error_stop(TXT("why eyes are not on the right side?"));
			}
		}
		{
			MEASURE_PERFORMANCE_COLOURED(store, Colour::orange);
			store(owner->access_render_pose_set(), nullptr, trackingState);
		}
		{
			MEASURE_PERFORMANCE_COLOURED(storeLastValid, Colour::yellow);
			owner->store_last_valid_render_pose_set();
		}
#ifdef DEBUG_READING_POSES
		{
			auto& pose = owner->access_render_pose_set();
			if (pose.view.is_set())
			{
				output(TXT("[system-openxr] advance render pose %.3f x %.3f, %.3f"), pose.view.get().get_translation().x, pose.view.get().get_translation().y, pose.view.get().get_orientation().to_rotator().yaw);
			}
		}
#endif
	}
	
	begunFrame = true;

#ifdef AN_VIVE
	return true; // always render for vive, otherwise will crash (nothing provided?)
#else
	return xrFrameSessionShouldRender;
#endif
}

void OpenXRImpl::begin_render_for_eye(System::Video3D* _v3d, Eye::Type _eye)
{
	auto& rmd = renderModeData[renderMode];
	
	auto& brfe = rmd.begunRenderForEye[_eye];

	if (!begunFrame)
	{
		return;
	}

	if (brfe.acquiredImageIndex.is_set())
	{
		return;
	}

	if (rmd.xrSwapchains.is_index_valid(_eye))
	{
#ifdef AN_LOG_FRAME_CALLS
		output_gl(TXT("being render for eye %i"), _eye);
#endif

		auto& xsc = rmd.xrSwapchains[_eye];

#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
#ifdef UPDATE_FOVEATION_EVERY_FRAME
		//every frame
		if (ext_xrUpdateSwapchainFB
#ifndef UPDATE_FOVEATION_EVERY_FRAME_CREATE_PROFILE
			&& rmd.xrFoveationProfile != XR_NULL_HANDLE
#endif
			)
		{
#ifdef UPDATE_FOVEATION_EVERY_FRAME_CREATE_PROFILE
			XrFoveationProfileFB xrFoveationProfile = rmd.xrFoveationProfile;
			if (xrFoveationAvailable && xrFoveationUsed && ext_xrCreateFoveationProfileFB)
			{
				XrFoveationLevelFB useFoveationLevel = XR_FOVEATION_LEVEL_NONE_FB;
				{
					auto vraf = get_available_functionality();
					float configLevel = MainConfig::global().get_video().vrFoveatedRenderingLevel;
					useFoveationLevel = (XrFoveationLevelFB)vraf.convert_foveated_rendering_config_to_level(configLevel);
				}

				XrFoveationLevelProfileCreateInfoFB levelProfileCreateInfo;
				memory_zero(levelProfileCreateInfo);
				levelProfileCreateInfo.type = XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB;
				levelProfileCreateInfo.level = min(useFoveationLevel, XR_FOVEATION_LEVEL_HIGH_FB);
				levelProfileCreateInfo.verticalOffset = 0.0f;
				levelProfileCreateInfo.dynamic = XR_FOVEATION_DYNAMIC_DISABLED_FB;
				levelProfileCreateInfo.next = nullptr;

				XrFoveationProfileCreateInfoFB profileCreateInfo;
				memory_zero(profileCreateInfo);
				profileCreateInfo.type = XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB;
				profileCreateInfo.next = &levelProfileCreateInfo;

				output(TXT("[system-openxr] create foveation profile, level %i"), levelProfileCreateInfo.level);
				check_openxr(ext_xrCreateFoveationProfileFB(xrSession, &profileCreateInfo, &xrFoveationProfile), TXT("create foveation profile"));
			}
#else
			XrFoveationProfileFB xrFoveationProfile = rmd.xrFoveationProfile;
#endif

			if (xrFoveationProfile)
			{
				output(TXT("[system-openxr] update swapchain with foveated profile"));
				XrSwapchainStateFoveationFB foveationUpdateState;
				memory_zero(foveationUpdateState);
				foveationUpdateState.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB;
				foveationUpdateState.profile = xrFoveationProfile;

#ifdef AN_LOG_FRAME_CALLS
				REMOVE_OR_CHANGE_BEFORE_COMMIT_ output_gl(TXT("update swapchain foveated rendering"));
#endif
				check_openxr(ext_xrUpdateSwapchainFB(xsc.xrSwapchain, (XrSwapchainStateBaseHeaderFB*)(&foveationUpdateState)), TXT("set foveation via update swapchain"));
			}

#ifdef UPDATE_FOVEATION_EVERY_FRAME_CREATE_PROFILE
			ext_xrDestroyFoveationProfileFB(xrFoveationProfile);
#endif
		}
#endif
#endif

		{
			uint32_t acquiredImageIndex = NONE;

			{
				{
					XrSwapchainImageAcquireInfo acquireInfo;
					memory_zero(acquireInfo);
					acquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
					acquireInfo.next = nullptr;
					check_openxr(xrAcquireSwapchainImage(xsc.xrSwapchain, &acquireInfo, &acquiredImageIndex), TXT("acquire swapchain image"));
				}

				{
					MEASURE_PERFORMANCE_COLOURED(xrWaitSwapchainImage, Colour::black);

					XrSwapchainImageWaitInfo waitInfo;
					memory_zero(waitInfo);
					waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
					waitInfo.next = nullptr;
					waitInfo.timeout = 1000000000; // nanoseconds!
					XrResult result = xrWaitSwapchainImage(xsc.xrSwapchain, &waitInfo);
					check_openxr(result, TXT("wait swapchain image"));
					while (result == XR_TIMEOUT_EXPIRED)
					{
						MEASURE_PERFORMANCE_COLOURED(xrWaitSwapchainImage_timeOut, Colour::black);
						warn(TXT("[system-openxr] waiting for swapchain image [%i:%i]"), renderMode, _eye);
						result = xrWaitSwapchainImage(xsc.xrSwapchain, &waitInfo);
						check_openxr(result, TXT("wait swapchain image"));
					}
				}

				PERFORMANCE_MARKER(Colour::yellow);
			}

			if (acquiredImageIndex != NONE)
			{
				brfe.acquiredImageIndex = acquiredImageIndex;
			}
		}

		if (xsc.withDepthSwapchain)
		{
			uint32_t acquiredDepthImageIndex = NONE;

			{
				{
					XrSwapchainImageAcquireInfo acquireInfo;
					memory_zero(acquireInfo);
					acquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
					acquireInfo.next = nullptr;
					check_openxr(xrAcquireSwapchainImage(xsc.xrDepthSwapchain, &acquireInfo, &acquiredDepthImageIndex), TXT("acquire swapchain image (depth)"));
				}

				{
					MEASURE_PERFORMANCE_COLOURED(xrWaitSwapchainImage_depth, Colour::black);

					XrSwapchainImageWaitInfo waitInfo;
					memory_zero(waitInfo);
					waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
					waitInfo.next = nullptr;
					waitInfo.timeout = 1000000000; // nanoseconds!
					XrResult result = xrWaitSwapchainImage(xsc.xrDepthSwapchain, &waitInfo);
					check_openxr(result, TXT("wait swapchain image"));
					while (result == XR_TIMEOUT_EXPIRED)
					{
						MEASURE_PERFORMANCE_COLOURED(xrWaitSwapchainImage_depth_timeOut, Colour::black);
						warn(TXT("[system-openxr] waiting for swapchain depth image [%i:%i]"), renderMode, _eye);
						result = xrWaitSwapchainImage(xsc.xrDepthSwapchain, &waitInfo);
						check_openxr(result, TXT("wait swapchain image"));
					}
				}

				PERFORMANCE_MARKER(Colour::yellow);
			}

			if (acquiredDepthImageIndex != NONE)
			{
				brfe.acquiredDepthImageIndex = acquiredDepthImageIndex;
			}
		}
	}
}

void OpenXRImpl::end_render_for_eye(Eye::Type _eye)
{
	auto& rmd = renderModeData[renderMode];
	auto& brfe = rmd.begunRenderForEye[_eye];
	if (rmd.xrSwapchains.is_index_valid(_eye))
	{
#ifdef AN_LOG_FRAME_CALLS
		output_gl(TXT("end render for eye %i"), _eye);
#endif

		auto& xsc = rmd.xrSwapchains[_eye];

		if (brfe.acquiredImageIndex.is_set())
		{
			XrSwapchainImageReleaseInfo releaseInfo;
			memory_zero(releaseInfo);
			releaseInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
			releaseInfo.next = nullptr;
			check_openxr(xrReleaseSwapchainImage(xsc.xrSwapchain, &releaseInfo), TXT("release swapchain image"));
			brfe.acquiredImageIndex.clear();
		}
	
		if (brfe.acquiredDepthImageIndex.is_set())
		{
			XrSwapchainImageReleaseInfo releaseInfo;
			memory_zero(releaseInfo);
			releaseInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
			releaseInfo.next = nullptr;
			check_openxr(xrReleaseSwapchainImage(xsc.xrDepthSwapchain, &releaseInfo), TXT("release swapchain image (depth)"));
			brfe.acquiredDepthImageIndex.clear();
		}
	}
}

void OpenXRImpl::read_tracking_state(OUT_ TrackingState& _trackingState, XrTime _atTime)
{
	if (_atTime == 0)
	{
		// ignore reading
		return;
	}

	// sync actions/controls
	{
		MEASURE_PERFORMANCE(syncActionsControls);

		XrActiveActionSet activeActionSet;
		memory_zero(activeActionSet);
		activeActionSet.actionSet = xrActionSet;
		activeActionSet.subactionPath = XR_NULL_PATH;

		XrActionsSyncInfo actionsSyncInfo;
		memory_zero(actionsSyncInfo);
		actionsSyncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
		actionsSyncInfo.countActiveActionSets = 1;
		actionsSyncInfo.activeActionSets = &activeActionSet;

		check_openxr(xrSyncActions(xrSession, &actionsSyncInfo), TXT("sync actions"));
	}

	// locate space head only if session is in focus or visible (otherwise it is not required and we may get frozen in xrLocateSpace)
	if (xrSessionInFocus || xrSessionVisible)
	{
		MEASURE_PERFORMANCE(locateSpaceView);

		XrSpaceLocation viewLocation;
		memory_zero(viewLocation);
		viewLocation.type = XR_TYPE_SPACE_LOCATION;
		viewLocation.next = nullptr;
		if (check_openxr(xrLocateSpace(xrViewPoseSpace, xrPlaySpace, _atTime, &viewLocation), TXT("locate space view")))
		{
			if (is_flag_set(viewLocation.locationFlags, XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			 && is_flag_set(viewLocation.locationFlags, XR_SPACE_LOCATION_POSITION_VALID_BIT)
			 && is_flag_set(viewLocation.locationFlags, XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)
			 && is_flag_set(viewLocation.locationFlags, XR_SPACE_LOCATION_POSITION_TRACKED_BIT)
				)
			{
				_trackingState.viewPose = get_transform_from(viewLocation.pose);
#ifdef DEBUG_READING_POSES
				output(TXT("[system-openxr] read raw view space % .3f x % .3f, % .3f"), _trackingState.viewPose.get().get_translation().x, _trackingState.viewPose.get().get_translation().y, _trackingState.viewPose.get().get_orientation().to_rotator().yaw);
#endif
			}
		}
	}

	// if not tracked, let the game do whatever it finds the best for given situation

	struct CheckActionState
	{
		OpenXRImpl const* openXR = nullptr;
		XrSession xrSession;

		bool is_active(XrAction _poseAction, XrPath _subactionPath)
		{
			XrActionStatePose actionStatePose;
			memory_zero(actionStatePose);
			actionStatePose.type = XR_TYPE_ACTION_STATE_POSE;

			XrActionStateGetInfo getActionStateInfo;
			memory_zero(getActionStateInfo);
			getActionStateInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
			getActionStateInfo.action = _poseAction;
			getActionStateInfo.subactionPath = _subactionPath;
			openXR->check_openxr(xrGetActionStatePose(xrSession, &getActionStateInfo, &actionStatePose), TXT("check action state"));

			return actionStatePose.isActive;
		}
	} checkActionState;
	checkActionState.openXR = this;
	checkActionState.xrSession = xrSession;

	for_count(int, handIdx, Hand::MAX)
	{
		MEASURE_PERFORMANCE(hand);

		XrSpaceLocation handAimLocation;
		XrSpaceLocation handGripLocation;

		memory_zero(handAimLocation);
		memory_zero(handGripLocation);

		handAimLocation.type = XR_TYPE_SPACE_LOCATION;
		handAimLocation.next = nullptr;
		handGripLocation.type = XR_TYPE_SPACE_LOCATION;
		handGripLocation.next = nullptr;

		XrResult locateAimLocationResult = XR_SUCCESS; //ignorable
		XrResult locateGripLocationResult = XR_SUCCESS; //ignorable
		{
			MEASURE_PERFORMANCE(prepareLocateSpaces);
			if (checkActionState.is_active(xrHandAimPoseAction, xrHandPaths[handIdx]))
			{
				locateAimLocationResult = xrLocateSpace(xrHandAimPoseSpaces[handIdx], xrPlaySpace, _atTime, &handAimLocation);
			}
			if (checkActionState.is_active(xrHandGripPoseAction, xrHandPaths[handIdx]))
			{
				locateGripLocationResult = xrLocateSpace(xrHandGripPoseSpaces[handIdx], xrPlaySpace, _atTime, &handGripLocation);
			}
		}

		bool locatedSpaces;
		{
			MEASURE_PERFORMANCE(prepareLocateSpaces);
			locatedSpaces = check_openxr(locateAimLocationResult, TXT("locate space aim hand")) &&
							check_openxr(locateGripLocationResult, TXT("locate space grip hand"));
		}
		if (locatedSpaces)
		{
			if (is_flag_set(handAimLocation.locationFlags, XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			 && is_flag_set(handAimLocation.locationFlags, XR_SPACE_LOCATION_POSITION_VALID_BIT)
			 && is_flag_set(handAimLocation.locationFlags, XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)
			 && is_flag_set(handAimLocation.locationFlags, XR_SPACE_LOCATION_POSITION_TRACKED_BIT)
			 && is_flag_set(handGripLocation.locationFlags, XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			 && is_flag_set(handGripLocation.locationFlags, XR_SPACE_LOCATION_POSITION_VALID_BIT)
			 && is_flag_set(handGripLocation.locationFlags, XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)
			 && is_flag_set(handGripLocation.locationFlags, XR_SPACE_LOCATION_POSITION_TRACKED_BIT)
				)
			{
				Transform handGrip = get_transform_from(handGripLocation.pose);
				Transform handAim = get_transform_from(handAimLocation.pose);
				_trackingState.handPoses[handIdx] = look_matrix(handGrip.get_translation(), handAim.get_axis(Axis::Forward), handAim.get_axis(Axis::Up)).to_transform();
			}
		}
	}

#ifdef AN_OPENXR_WITH_HAND_TRACKING
	if (handTrackingAvailable && handTrackingInitialised)
	{
		MEASURE_PERFORMANCE(handTracking);

		bool handTrackingChanged = false;
		for_count(int, handIdx, Hand::MAX)
		{
			XrHandJointLocationEXT jointLocations[XR_HAND_JOINT_COUNT_EXT];
			//XrHandJointVelocityEXT jointVelocities[XR_HAND_JOINT_COUNT_EXT];

			//XrHandJointVelocitiesEXT velocities;
			//memory_zero(velocities);
			//velocities.type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
			//velocities.jointCount = XR_HAND_JOINT_COUNT_EXT;
			//velocities.jointVelocities = jointVelocities;

			XrHandJointLocationsEXT locations;
			memory_zero(locations);
			locations.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
			//locations.next = &velocities;
			locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
			locations.jointLocations = jointLocations;

			XrHandJointsLocateInfoEXT locateInfo;
			memory_zero(locateInfo);
			locateInfo.type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT;
			locateInfo.baseSpace = xrPlaySpace;
			locateInfo.time = _atTime;

			XrHandTrackingAimStateFB aimState;
			memory_zero(aimState);
			aimState.type = XR_TYPE_HAND_TRACKING_AIM_STATE_FB;
			if (handTrackingAimSupported)
			{
				locations.next = &aimState;
			}

			bool usingHandTrackingNow = false;
			auto& xrHandTracker = xrHandTrackers[handIdx];
			if (check_openxr(ext_xrLocateHandJointsEXT(xrHandTracker, &locateInfo, &locations), TXT("locate hand joints")))
			{
				if (locations.isActive)
				{
					usingHandTrackingNow = true;
					auto& handTrackingState = _trackingState.handTracking[handIdx];
					handTrackingState.active = true;
					Transform handSpace = get_transform_from(jointLocations[XR_HAND_JOINT_PALM_EXT].pose);
					handTrackingState.handPose = handSpace;
					handTrackingState.jointsHS.set_size(XR_HAND_JOINT_COUNT_EXT);
					for_every(joint, handTrackingState.jointsHS)
					{
						Transform readJoint = get_transform_from(jointLocations[for_everys_index(joint)].pose);
						Transform inHandSpace = handSpace.to_local(readJoint);
						*joint = inHandSpace;
					}

					if (handTrackingAimSupported)
					{
						if (is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB) ||
							is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_COMPUTED_BIT_FB))
						{
							handTrackingState.aimPose = get_transform_from(aimState.aimPose);
						}
						if (is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB))
						{
							handTrackingState.pointerPinch = aimState.pinchStrengthIndex;
						}
						if (is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB))
						{
							handTrackingState.middlePinch = aimState.pinchStrengthMiddle;
						}
						if (is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB))
						{
							handTrackingState.ringPinch = aimState.pinchStrengthRing;
						}
						if (is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB))
						{
							handTrackingState.pinkyPinch = aimState.pinchStrengthLittle;
						}
						if (is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_SYSTEM_GESTURE_BIT_FB))
						{
							handTrackingState.systemGesture = true;
						}
						if (is_flag_set(aimState.status, XR_HAND_TRACKING_AIM_MENU_PRESSED_BIT_FB))
						{
							handTrackingState.menuPressed = true;
						}
					}
				}
			}
			handTrackingChanged |= owner->set_using_hand_tracking(handIdx, usingHandTrackingNow);
		}
		if (handTrackingChanged)
		{
			owner->decide_hands();
			owner->mark_new_controllers();
		}
	}
#endif
}

void OpenXRImpl::store(VRPoseSet& _poseSet, OPTIONAL_ VRControls* _controls, TrackingState const& _trackingState)
{
	if (_trackingState.viewPose.is_set())
	{
		_poseSet.rawViewAsRead = _trackingState.viewPose.get().to_world(viewInReadViewPoseSpace);
	}
#ifdef AN_INSPECT_VR_INVALID_ORIGIN_CONTINUOUS
	else
	{
		output(TXT("[vr_invalid_origin] no view provided"));
	}
#endif

	for_count(int, i, Hands::Count)
	{
		bool useHandTrackingNow = false;
#ifdef AN_OPENXR_WITH_HAND_TRACKING
		if (handTrackingAvailable && handTrackingInitialised)
		{
			useHandTrackingNow = _trackingState.handTracking[i].active;
		}
#endif

		{
			if (!useHandTrackingNow && 
				_controls)
			{
				auto& controls = _controls->hands[i];

				controls.trackpadTouchDelta = Vector2::zero; // reset delta, we will update it if possible

				controls.upsideDown.clear();
				controls.insideToHead.clear();
				controls.pointerPinch.clear();
				controls.middlePinch.clear();
				controls.ringPinch.clear();
				controls.pinkyPinch.clear();

				struct ProcessMultipleTouch
				{
					Optional<bool>& touch;
					ProcessMultipleTouch(Optional<bool>& _touch) : touch(_touch) {}
					bool readAnything = false;
					void process(Optional<bool> const& input)
					{
						if (input.is_set())
						{
							readAnything = true;
							if (input.get())
							{
								touch = true;
							}
						}
					}

					void end()
					{
						if (!touch.is_set() && readAnything)
						{
							touch = false;
						}
					}
				};

				struct ReadInput
				{
					OpenXRImpl const* openXR = nullptr;
					XrSession xrSession;
					XrPath xrHandPath;

					Optional<bool> get_bool(XrAction const & _xrAction)
					{
						XrActionStateGetInfo getInfo;
						memory_zero(getInfo);
						getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
						getInfo.next = nullptr;
						getInfo.action = _xrAction;
						getInfo.subactionPath = xrHandPath;

						XrActionStateBoolean result;
						memory_zero(result); 
						result.type = XR_TYPE_ACTION_STATE_BOOLEAN;
						result.next = nullptr;

						if (openXR->check_openxr(xrGetActionStateBoolean(xrSession, &getInfo, &result), TXT("get action state bool")))
						{
							if (result.isActive)
							{
								return result.currentState;
							}
						}
						return NP;
					}

					Optional<float> get_float(XrAction const& _xrAction)
					{
						XrActionStateGetInfo getInfo;
						memory_zero(getInfo);
						getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
						getInfo.next = nullptr;
						getInfo.action = _xrAction;
						getInfo.subactionPath = xrHandPath;

						XrActionStateFloat result;
						memory_zero(result);
						result.type = XR_TYPE_ACTION_STATE_FLOAT;
						result.next = nullptr;

						if (openXR->check_openxr(xrGetActionStateFloat(xrSession, &getInfo, &result), TXT("get action state float")))
						{
							if (result.isActive)
							{
								return result.currentState;
							}
						}
						return NP;
					}

					Optional<Vector2> get_vector2(XrAction const& _xrAction)
					{
						XrActionStateGetInfo getInfo;
						memory_zero(getInfo);
						getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
						getInfo.next = nullptr;
						getInfo.action = _xrAction;
						getInfo.subactionPath = xrHandPath;

						XrActionStateVector2f result;
						memory_zero(result);
						result.type = XR_TYPE_ACTION_STATE_VECTOR2F;
						result.next = nullptr;

						if (openXR->check_openxr(xrGetActionStateVector2f(xrSession, &getInfo, &result), TXT("get action state vector2")))
						{
							if (result.isActive)
							{
								return get_vector2_from(result.currentState);
							}
						}
						return NP;
					}

				} readInput;

				readInput.openXR = this;
				readInput.xrSession = xrSession;
				readInput.xrHandPath = xrHandPaths[i];
				
				controls.trigger = max(readInput.get_float(xrTriggerPressAsFloatAction).get(0.0f), readInput.get_bool(xrTriggerPressAsBoolAction).get(false) ? 1.0f : 0.0f);
				controls.grip = max(readInput.get_float(xrGripPressAsFloatAction).get(0.0f), readInput.get_bool(xrGripPressAsBoolAction).get(false) ? 1.0f : 0.0f);
				controls.buttons.primary = readInput.get_bool(xrPrimaryPressAsBoolAction).get(false);
				controls.buttons.secondary = readInput.get_bool(xrSecondaryPressAsBoolAction).get(false);
				controls.buttons.systemMenu = readInput.get_bool(xrMenuPressAsBoolAction).get(false);

				controls.joystick = readInput.get_vector2(xrThumbstickAsVector2Action).get(Vector2::zero);
				controls.buttons.joystickPress = readInput.get_bool(xrThumbstickPressAsBoolAction).get(false);

				Optional<bool> triggerTouch = readInput.get_bool(xrTriggerTouchAsBoolAction);
				Optional<bool> thumbDownTouch;
				ProcessMultipleTouch thumbMultipleTouchProcessor(thumbDownTouch);
				thumbMultipleTouchProcessor.process(readInput.get_bool(xrThumbRestTouchAsBoolAction));
				thumbMultipleTouchProcessor.process(readInput.get_bool(xrPrimaryTouchAsBoolAction));
				thumbMultipleTouchProcessor.process(readInput.get_bool(xrSecondaryTouchAsBoolAction));
				thumbMultipleTouchProcessor.process(readInput.get_bool(xrMenuTouchAsBoolAction));
				thumbMultipleTouchProcessor.process(readInput.get_bool(xrThumbstickTouchAsBoolAction));
				thumbMultipleTouchProcessor.end();

				// this setup works for oculus touch
				if (triggerTouch.is_set() && triggerTouch.get() == false && controls.grip.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD)
				{
					controls.pose = 1.0f;
					controls.thumb = thumbDownTouch.get(true) ? 1.0f : 0.0f; // if not present, assume we always have thumb down
					controls.pointer = 0.0f;
					controls.middle = 1.0f;
					controls.ring = 1.0f;
					controls.pinky = 1.0f;
					controls.grip = 1.0f;
				}
				else if (thumbDownTouch.is_set() && thumbDownTouch.get() == false && controls.grip.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD)
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
					float makeFist = controls.grip.get(0.0f);
					controls.pose = 0.0f;
					controls.thumb = makeFist;
					controls.pointer = makeFist;
					controls.middle = makeFist;
					controls.ring = makeFist;
					controls.pinky = makeFist;
					controls.grip = makeFist;
				}

				{
					controls.buttons.trackpad = readInput.get_bool(xrTrackpadPressAsBoolAction).get(false);
					controls.buttons.trackpadTouch = readInput.get_bool(xrTrackpadTouchAsBoolAction).get(false);

					Vector2 trackpad = readInput.get_vector2(xrTrackpadAsVector2Action).get(Vector2::zero);
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

				controls.do_auto_buttons();

				todo_note(TXT("hand tracking?"));
				controls.buttons.systemGestureProcessing = false;
			}
			
			if (useHandTrackingNow)
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

				if (_controls)
				{
					auto& controls = _controls->hands[i];
					controls.do_auto_buttons();
				}

#ifdef AN_OPENXR_WITH_HAND_TRACKING
				{
					auto& handTracking = _trackingState.handTracking[i];
					if (_controls)
					{
						auto& controls = _controls->hands[i];
						controls.pointerPinch = handTracking.pointerPinch.get(0.0f);
						controls.middlePinch = handTracking.middlePinch.get(0.0f);
						controls.ringPinch = handTracking.ringPinch.get(0.0f);
						controls.pinkyPinch = handTracking.pinkyPinch.get(0.0f);
						controls.buttons.systemGestureProcessing = handTracking.systemGesture;
						if (owner->get_dominant_hand() != i)
						{
							controls.buttons.systemMenu = handTracking.menuPressed;
						}
						else
						{
							controls.buttons.systemMenu = false;
						}
					}
				}
#endif

				if (_controls)
				{
					auto& controls = _controls->hands[i];
					controls.do_auto_buttons();
				}
			}
		}

		Transform vrHandPose = Transform::identity;
		Transform vrHandRoot = vrHandPose;
		Transform vrHandPointer = vrHandPose;
		bool writeVRHandPose = false;
		bool writeVRHandRoot = false;
#ifdef AN_OPENXR_WITH_HAND_TRACKING
		bool writeVRHandPointer = false;
#endif

		if (useHandTrackingNow)
		{
#ifdef AN_OPENXR_WITH_HAND_TRACKING
			auto& handTracking = _trackingState.handTracking[i];
			if (handTracking.handPose.is_set())
			{
				vrHandPose = handTracking.handPose.get();
				vrHandRoot = vrHandPose;
				writeVRHandPose = true;
				writeVRHandRoot = true;

				if (!handTracking.jointsHS.is_empty())
				{
					auto& pose = _poseSet.hands[i];

					// read bones inside
					for_count(int, b, min(VRHandPose::MAX_RAW_BONES, handTracking.jointsHS.get_size()))
					{
						pose.rawBonesLS[b].clear();
						pose.rawBonesRS[b] = handTracking.jointsHS[b]; // already in hand pose which is also our hand root (see above)
					}
				}
			}
			else
			{
				writeVRHandRoot = false;
			}
			if (handTracking.aimPose.is_set())
			{
				vrHandPointer = handTracking.aimPose.get();
				writeVRHandPointer = true;
			}
			else
			{
				writeVRHandPointer = false;
			}
#endif
		}
		else if (_trackingState.handPoses[i].is_set())
		{
			vrHandPose = _trackingState.handPoses[i].get();
			vrHandRoot = vrHandPose;
			vrHandPointer = vrHandPose;
			writeVRHandPose = true;
			writeVRHandRoot = true;
		}

		{
			auto& pose = _poseSet.hands[i];

			if (writeVRHandPose)
			{
				pose.rawPlacementAsRead = vrHandPose;
			}
			else
			{
				pose.rawPlacementAsRead.clear();
			}
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

bool OpenXRImpl::end_render(System::Video3D* _v3d, ::System::RenderTargetPtr* _eyeRenderTargets)
{
	if (!is_ok())
	{
		return false;
	}
	if (!xrSessionRunning &&
		!begunFrame) // continue if frame pending
	{
		return false;
	}

	::System::RenderTarget::unbind_to_none();

	bool displayResult = true;

	if (xrSessionRunning)
	{
		auto& rmd = renderModeData[renderMode];
		for_every(xsc, rmd.xrSwapchains)
		{
			int eyeIdx = for_everys_index(xsc);
			if (eyeIdx >= Eye::Count)
			{
				break;
			}

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
			::System::FoveatedRenderingSetup::handle_rendering_test_grid(eyeIdx);
#endif

			// if already begun, we're good, we have acquired images
			begin_render_for_eye(_v3d, (Eye::Type)eyeIdx);
			auto& brfe = rmd.begunRenderForEye[eyeIdx];

			if (!_eyeRenderTargets[eyeIdx].is_set())
			{
				// we should already render it to the actual render target
				an_assert(IVR::get()->does_use_direct_to_vr_rendering());
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

					::System::TextureID const colourTextureId = xsc->images[imageIndex].image;

					DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, xsc->imageFrameBufferIDs[useSingleFrameBuffer? 0 : imageIndex]); AN_GL_CHECK_FOR_ERROR
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

					VectorInt2 renderSize = rmd.viewSize[eyeIdx];

					_v3d->set_viewport(VectorInt2::zero, renderSize);
					_v3d->set_near_far_plane(0.02f, 100.0f);

					_v3d->set_2d_projection_matrix_ortho();
					_v3d->access_model_view_matrix_stack().clear();

					_eyeRenderTargets[eyeIdx]->render(0, _v3d, Vector2::zero, renderSize.to_vector2()); AN_GL_CHECK_FOR_ERROR

					DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, xsc->imageFrameBufferIDs[useSingleFrameBuffer ? 0 : imageIndex]); AN_GL_CHECK_FOR_ERROR
					DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0); AN_GL_CHECK_FOR_ERROR
				}
			}

			// will close them
			end_render_for_eye((Eye::Type)eyeIdx);
		}

		::System::Video3D::get()->invalidate_bound_frame_buffer_info();
		::System::RenderTarget::unbind_to_none();


#ifdef AN_LOG_FRAME_CALLS
		output_gl(TXT("[system-openxr]::display - done rendering, flush and end frame"));
#endif

		if (false)
		{
			MEASURE_PERFORMANCE(flush);
			DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR
		}

		XrCompositionLayerProjection projectionLayer;
		memory_zero(projectionLayer);
		projectionLayer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
		projectionLayer.next = nullptr;
		projectionLayer.layerFlags = 0;
		projectionLayer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
		projectionLayer.space = xrPlaySpace;
		projectionLayer.viewCount = xrViews.get_size();
		projectionLayer.views = rmd.xrProjectionViews.begin();
		for_every(v, rmd.xrProjectionViews)
		{
			auto& xrView = xrViews[for_everys_index(v)];
			v->fov = xrView.fov;
			v->pose = xrView.pose;
		}

		int submittedLayerCount = 1;
		XrCompositionLayerBaseHeader const* submittedLayers[1] = { (const XrCompositionLayerBaseHeader* const)&projectionLayer };

		//if ((view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
		//	printf("submitting 0 layers because orientation is invalid\n");
		//	submitted_layer_count = 0;
		//}

		if (!xrFrameSessionShouldRender)
		{
			submittedLayerCount = 0;
		}	

#ifdef AN_LOG_FRAME_CALLS
		{
			static int endFrame = 0;
			output_gl(TXT("[system-openxr] : xrEndFrame %i"), endFrame);
			++endFrame;
		}
#endif

		MEASURE_PERFORMANCE_COLOURED(xrEndFrame, Colour::black);

		XrFrameEndInfo frameEndInfo;
		memory_zero(frameEndInfo);
		frameEndInfo.type = XR_TYPE_FRAME_END_INFO;
		frameEndInfo.displayTime = predictedDisplayTime;
		frameEndInfo.layerCount = submittedLayerCount;
		frameEndInfo.layers = submittedLayers;
		frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		frameEndInfo.next = nullptr;
		XrResult result = xrEndFrame(xrSession, &frameEndInfo);
		if (!check_openxr(result, TXT("end frame")))
		{
			displayResult = false;
		}
		PERFORMANCE_MARKER(Colour::red);
	}

	begunFrame = false;

	return displayResult;
}

void OpenXRImpl::setup_rendering_on_init_video(::System::Video3D* _v3d)
{
	an_assert(is_ok(), TXT("openxr should be working"));

	recommendedViewSize.set_size(xrConfigViews.get_size());
	for_every(xcv, xrConfigViews)
	{
		int eyeIdx = for_everys_index(xcv);

		if (eyeIdx >= Eye::Count)
		{
			break;
		}
		
		recommendedViewSize[eyeIdx].x = xcv->recommendedImageRectWidth;
		recommendedViewSize[eyeIdx].y = xcv->recommendedImageRectHeight;

		for_count(int, rmIdx, InternalRenderMode::COUNT)
		{
			auto& rmd = renderModeData[rmIdx];

			// fill with anything, we'll be updating it in begin_render
			rmd.renderInfo.fovPort[eyeIdx].left = -45.0f;
			rmd.renderInfo.fovPort[eyeIdx].right = 45.0f;
			rmd.renderInfo.fovPort[eyeIdx].up = 45.0f;
			rmd.renderInfo.fovPort[eyeIdx].down = -45.0f;

			// store render info
			rmd.renderInfo.eyeResolution[eyeIdx].x = xcv->recommendedImageRectWidth;
			rmd.renderInfo.eyeResolution[eyeIdx].y = xcv->recommendedImageRectHeight;
			rmd.renderInfo.fov[eyeIdx] = rmd.renderInfo.fovPort[eyeIdx].get_fov(); // this is invalid
			rmd.renderInfo.aspectRatio[eyeIdx] = aspect_ratio(rmd.renderInfo.eyeResolution[eyeIdx]);
			rmd.renderInfo.lensCentreOffset[eyeIdx] = rmd.renderInfo.fovPort[eyeIdx].get_lens_centre_offset(); // this is invalid

			rmd.renderInfo.eyeOffset[eyeIdx] = Transform::identity;
		}
	}

	if (renderModeData[InternalRenderMode::Normal].renderInfo.eyeOffset[Eye::Left].get_translation().x > 0.0f)
	{
		error_stop(TXT("why eyes are not on the right side?"));
	}
}

void OpenXRImpl::force_bounds_rendering(bool _force)
{
	todo_note(TXT("force rendering of bounds? how?"));
}

float OpenXRImpl::calculate_ideal_expected_frame_time() const
{
	if (displayRefreshRate.get(0.0f) != 0.0f)
	{
		return 1.0f / max(10.0f, displayRefreshRate.get());
	}
	else if (displayRefreshRateFromWaitFrame.get(0.0f) != 0.0f)
	{
		return 1.0f / max(10.0f, displayRefreshRateFromWaitFrame.get());
	}
	else
	{
		return 1.0f / systemBasedRefreshRate.get(DEFAULT_DISPLAY_REFRESH_RATE);
	}
}

float OpenXRImpl::update_expected_frame_time()
{
	float idealExpectedFrameTime = calculate_ideal_expected_frame_time();

	float expectedFrameTime = deltaTimeFromSync;

	expectedFrameTime = round_to(expectedFrameTime, idealExpectedFrameTime); // it should be a multiplication of ideal, we actually should only be at x1 or x2
	expectedFrameTime = max(expectedFrameTime, idealExpectedFrameTime); // can't be shorter/faster

	return expectedFrameTime;
}

DecideHandsResult::Type OpenXRImpl::decide_hands(OUT_ int& _leftHand, OUT_ int& _rightHand)
{
	// openxr handles this for us
	_leftHand = 0;
	_rightHand = 1;
	return DecideHandsResult::Decided;
}

void OpenXRImpl::update_hands_controls_source()
{
	struct ReadInteractionProfileState
	{
		OpenXRImpl const* openXR = nullptr;
		XrInstance xrInstance = XR_NULL_HANDLE;
		XrSession xrSession = XR_NULL_HANDLE;
		Optional<XrPath> get_active(XrPath _topLeveLPath)
		{
			if (xrSession != XR_NULL_HANDLE)
			{
				XrInteractionProfileState interactionProfileState;
				memory_zero(interactionProfileState);
				interactionProfileState.type = XR_TYPE_INTERACTION_PROFILE_STATE;
				interactionProfileState.next = nullptr;
				if (openXR->check_openxr(xrGetCurrentInteractionProfile(xrSession, _topLeveLPath, &interactionProfileState), TXT("get current interation profile")))
				{
					if (interactionProfileState.interactionProfile != 0)
					{
						return interactionProfileState.interactionProfile;
					}
				}
			}
			return NP;
		}
	} readInteractionProfileState;
	readInteractionProfileState.openXR = this;
	readInteractionProfileState.xrInstance = xrInstance;
	readInteractionProfileState.xrSession = xrSession;

	// we depend on being forced
	for_count(int, handIdx, Hand::MAX)
	{
		String useControllerName = system.name;
		Optional<XrPath> usedProfile = readInteractionProfileState.get_active(xrHandPaths[handIdx]);
		if (usedProfile.is_set())
		{
			if (usedProfile.get() == xrInputProfileOculusTouch) useControllerName = hardcoded TXT("oculusTouchController");
			if (usedProfile.get() == xrInputProfileValveIndex) useControllerName = hardcoded TXT("valveIndexController");
			if (usedProfile.get() == xrInputProfileHTCVive) useControllerName = hardcoded TXT("htcViveController");
			if (usedProfile.get() == xrInputProfileMicrosoftMotion) useControllerName = hardcoded TXT("microsoftMotionController");
			if (usedProfile.get() == xrInputProfileHPMixedReality) useControllerName = hardcoded TXT("hpMixedRealityController");
			if (usedProfile.get() == xrInputProfileHTCViveCosmos) useControllerName = hardcoded TXT("htcViveCosmosController");
			if (usedProfile.get() == xrInputProfileHTCViveFocus3) useControllerName = hardcoded TXT("htcViveFocus3Controller");
			if (usedProfile.get() == xrInputProfilePicoNeo3Controller) useControllerName = hardcoded TXT("picoNeo3Controller"); // used also for pico 4, see _baseConfig.xml
		}
		owner->access_controls().hands[handIdx].source = VR::Input::Devices::find(String::empty(), system.name, useControllerName);
	}
}

void OpenXRImpl::draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font)
{
}

void OpenXRImpl::update_render_scaling(REF_ float & _scale)
{
	static float prevTargetScale = 1.0f;
	static ::System::TimeStamp prevTargetScaleTS;
	static float prevScale = 1.0f;
	static ::System::TimeStamp prevScaleTS;
	float targetScale = 1.0f;
	if (owner->get_render_mode() == RenderMode::HiRes)
	{
		// force to full
		targetScale = 1.0f;
		_scale = 1.0f;
	}
	else
	{
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
		if (! ::System::FoveatedRenderingSetup::is_rendering_test_grid())
#endif
		if (MainConfig::global().get_video().allowAutoAdjustForVR)
		{
			float coef = lerp(0.5f, 1.0f, min(performanceStats.gpu.rendering, performanceStats.gpu.thermal));
			coef = min(coef, performanceStats.frameDropsBased.fineDynamicScaleRenderTarget);
			targetScale *= max(0.5f, coef);
		}
	}

	float blendTime = targetScale > _scale ? 5.0f : 2.5f;
	_scale = blend_to_using_speed_based_on_time(_scale, targetScale, blendTime, ::System::Core::get_raw_delta_time());
	_scale = clamp(_scale, 0.5f, 1.0f);

	if (prevTargetScale != targetScale && prevTargetScaleTS.get_time_since() > 0.1f)
	{
		output(TXT("[system-openxr] changed render scaling target %.0f%% to %.0f%% (gpu.rendering:%.0f%%, gpu.thermal:%.0f%%, fineScaling:%.0f%%)"), prevTargetScale * 100.0f, targetScale * 100.0f,
			performanceStats.gpu.rendering * 100.0f,
			performanceStats.gpu.thermal * 100.0f,
			performanceStats.frameDropsBased.fineDynamicScaleRenderTarget * 100.0f);
		prevTargetScale = targetScale;
		prevTargetScaleTS.reset();
	}
	if (prevScale != _scale && prevScaleTS.get_time_since() > 0.1f)
	{
		output(TXT("[system-openxr] changed render scaling %.0f%% to %.0f%%"), prevScale * 100.0f, _scale * 100.0f);
		prevScale = _scale;
		prevScaleTS.reset();
	}
}

void OpenXRImpl::set_render_mode(RenderMode::Type _mode)
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

AvailableFunctionality OpenXRImpl::get_available_functionality() const
{
	AvailableFunctionality vraf;
#ifdef AN_ANDROID
	vraf.renderToScreen = false; // no screen on quest
	vraf.postProcess = false;
#else
	vraf.renderToScreen = true;
	vraf.postProcess = true;
#endif
	vraf.directToVR = true;
#ifdef OWN_FOVEATED_IMPLEMENTATION
	if (::System::VideoGLExtensions::get().QCOM_texture_foveated)
	{
		vraf.foveatedRenderingNominal = ::System::FoveatedRenderingSetup::get_nominal_setup();
		vraf.foveatedRenderingMax = ::System::FoveatedRenderingSetup::get_max_setup();
		vraf.foveatedRenderingMaxDepth = 6;
	}
#endif
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_FB
	if (xrFoveationAvailable)
	{
		vraf.foveatedRenderingNominal = XR_FOVEATION_LEVEL_HIGH_FB;
		vraf.foveatedRenderingMax = XR_FOVEATION_LEVEL_HIGH_FB;
		vraf.foveatedRenderingMaxDepth = 0;
	}
#endif
#ifdef OPENXR_FOVEATED_IMPLEMENTATION_HTC
	if (xrFoveationAvailable)
	{
		vraf.foveatedRenderingNominal = XR_FOVEATION_MODE_DYNAMIC_HTC;
		vraf.foveatedRenderingMax = XR_FOVEATION_MODE_DYNAMIC_HTC;
		vraf.foveatedRenderingMaxDepth = 0;
	}
#endif
	return vraf;
}

void OpenXRImpl::set_perf_thread(int _type, int _threadId, tchar const* _what)
{
#ifdef AN_ANDROID
	if (ext_xrSetAndroidApplicationThreadKHR)
	{
		output(TXT("[system-openxr] set perf thread %S is %i"), _what, _threadId);
		check_openxr(ext_xrSetAndroidApplicationThreadKHR(xrSession, (XrAndroidThreadTypeKHR)_type, _threadId), TXT("update perf thread %S"), _what);
	}
	else
	{
		output(TXT("[system-openxr] no extension to set perf thread %S to %i"), _what, _threadId);
	}
#endif
}

void OpenXRImpl::update_perf_threads()
{
#ifdef AN_ANDROID
	Concurrency::ScopedSpinLock lock(perfThreadLock, true);
	if (ext_xrSetAndroidApplicationThreadKHR)
	{
		if (mainThreadTid.is_set())
		{
			set_perf_thread(XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR, mainThreadTid.get(), TXT("app"));
		}
		if (renderThreadTid.is_set())
		{
			set_perf_thread(XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR, renderThreadTid.get(), TXT("rnd"));
		}
		/*
		for_every(tid, allThreadsTid)
		{
			if (//(!mainThreadTid.is_set() || *tid != mainThreadTid.get()) &&
				(!renderThreadTid.is_set() || *tid != renderThreadTid.get()))
			{
				set_perf_thread(XR_ANDROID_THREAD_TYPE_APPLICATION_WORKER_KHR, *tid, TXT("app-worker"));
			}
		}
		*/
		{
			LogInfoContext log;
			log_perf_threads_impl(log);
			log.output_to_output();
		}
	}
#endif
}

void OpenXRImpl::log_performance_stats() const
{
	output(TXT("[system-openxr] perf cpu: c:%03i r:%03i t:%03i"), TypeConversions::Normal::f_i_closest(performanceStats.cpu.compositing * 100.0f)
																, TypeConversions::Normal::f_i_closest(performanceStats.cpu.rendering * 100.0f)
																, TypeConversions::Normal::f_i_closest(performanceStats.cpu.thermal * 100.0f));
	output(TXT("[system-openxr] perf gpu: c:%03i r:%03i t:%03i"), TypeConversions::Normal::f_i_closest(performanceStats.gpu.compositing * 100.0f)
																, TypeConversions::Normal::f_i_closest(performanceStats.gpu.rendering * 100.0f)
																, TypeConversions::Normal::f_i_closest(performanceStats.gpu.thermal * 100.0f));
}

void OpenXRImpl::set_processing_levels_impl(Optional<float> const& _cpuLevel, Optional<float> const& _gpuLevel, bool _temporarily)
{
	struct SanePerfSetttingsLevel
	{
		static XrPerfSettingsLevelEXT process(float _level)
		{
			int leveli = TypeConversions::Normal::f_i_closest(_level * (float)XR_PERF_SETTINGS_LEVEL_BOOST_EXT - 5.0f);
			if (leveli > (XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT + XR_PERF_SETTINGS_LEVEL_BOOST_EXT) / 2)
			{
				return XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
			}
			if (leveli > (XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT + XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT) / 2)
			{
				return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
			}
			if (leveli < (XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT + XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT) / 2)
			{
				return XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
			}
			return XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
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
	if (ext_xrPerfSettingsSetPerformanceLevelEXT && xrSession != XR_NULL_HANDLE)
	{
		output(TXT("[system-openxr] perf set, cpu:%i, gpu:%i %S"), cpuLevel, gpuLevel, _temporarily? TXT("(temporarily)") : TXT(""));
		check_openxr(ext_xrPerfSettingsSetPerformanceLevelEXT(xrSession, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, useCPULevel), TXT("set perf settings cpu"));
		check_openxr(ext_xrPerfSettingsSetPerformanceLevelEXT(xrSession, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, useGPULevel), TXT("set perf settings gpu"));
	}
}

void OpenXRImpl::I_am_perf_thread_main_impl()
{
#ifdef AN_ANDROID
	int tid = gettid();
	if (!mainThreadTid.is_set() || mainThreadTid.get() != tid)
	{
		Concurrency::ScopedSpinLock lock(perfThreadLock, true);
		mainThreadTid = tid;
		allThreadsTid.push_back_unique(tid);
		output(TXT("[system-openxr] I am perf thread (app) %i"), tid);
		set_perf_thread(XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR, tid, TXT("app"));
		//update_perf_threads();
	}
#endif
}

void OpenXRImpl::I_am_perf_thread_other_impl()
{
#ifdef AN_ANDROID
	int tid = gettid();
	if (!allThreadsTid.does_contain(tid))
	{
		Concurrency::ScopedSpinLock lock(perfThreadLock, true);
		allThreadsTid.push_back(tid);
		output(TXT("[system-openxr] I am perf thread (app-other) %i"), tid);
		//update_perf_threads();
	}
#endif
}

void OpenXRImpl::I_am_perf_thread_render_impl()
{
#ifdef AN_ANDROID
	int tid = gettid();
	if (!renderThreadTid.is_set() || renderThreadTid.get() != tid)
	{
		Concurrency::ScopedSpinLock lock(perfThreadLock, true);
		renderThreadTid = tid;
		allThreadsTid.push_back_unique(tid);
		output(TXT("[system-openxr] I am perf thread (rnd) %i"), tid); 
		set_perf_thread(XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR, tid, TXT("rnd"));
		//update_perf_threads();
	}
#endif
}

void OpenXRImpl::log_perf_threads_impl(LogInfoContext& _log)
{
#ifdef AN_ANDROID
	Concurrency::ScopedSpinLock lock(perfThreadLock, true);
	Concurrency::ThreadManager::log_thread_info(_log, true);
		for_every(tid, allThreadsTid)
	{
		Concurrency::ThreadManager::log_thread_info(_log, false, *tid,
			(mainThreadTid.is_set() && mainThreadTid.get() == *tid)? TXT("app") :
			((renderThreadTid.is_set() && renderThreadTid.get() == *tid) ? TXT("rnd") : TXT("   "))
			);
	}
#endif
}

int OpenXRImpl::translate_bone_index(VRHandBoneIndex::Type _index) const
{
	if (_index == VRHandBoneIndex::Wrist) return XR_HAND_JOINT_WRIST_EXT;
	if (_index == VRHandBoneIndex::ThumbBase) return XR_HAND_JOINT_THUMB_PROXIMAL_EXT;
	if (_index == VRHandBoneIndex::PointerBase) return XR_HAND_JOINT_INDEX_PROXIMAL_EXT;
	if (_index == VRHandBoneIndex::MiddleBase) return XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT;
	if (_index == VRHandBoneIndex::RingBase) return XR_HAND_JOINT_RING_PROXIMAL_EXT;
	if (_index == VRHandBoneIndex::PinkyBase) return XR_HAND_JOINT_LITTLE_PROXIMAL_EXT;
	if (_index == VRHandBoneIndex::ThumbMid) return XR_HAND_JOINT_THUMB_DISTAL_EXT;
	if (_index == VRHandBoneIndex::PointerMid) return XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT;
	if (_index == VRHandBoneIndex::MiddleMid) return XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT;
	if (_index == VRHandBoneIndex::RingMid) return XR_HAND_JOINT_RING_INTERMEDIATE_EXT;
	if (_index == VRHandBoneIndex::PinkyMid) return XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT;
	if (_index == VRHandBoneIndex::ThumbTip) return XR_HAND_JOINT_THUMB_TIP_EXT;
	if (_index == VRHandBoneIndex::PointerTip) return XR_HAND_JOINT_INDEX_TIP_EXT;
	if (_index == VRHandBoneIndex::MiddleTip) return XR_HAND_JOINT_MIDDLE_TIP_EXT;
	if (_index == VRHandBoneIndex::RingTip) return XR_HAND_JOINT_RING_TIP_EXT;
	if (_index == VRHandBoneIndex::PinkyTip) return XR_HAND_JOINT_LITTLE_TIP_EXT;

	return NONE;
}

void OpenXRImpl::test_loop()
{
	// Run a per-frame loop.
	::System::TimeStamp tsWaitFrame;
	::System::TimeStamp tsBeginFrame;
	while (true)
	{
		advance_events();

		// Wait for a new frame.
		XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
		XrFrameState frameState{ XR_TYPE_FRAME_STATE };
		output(TXT("[system-openxr] : last xrWaitFrame returned %.3fms ago"), tsWaitFrame.get_time_since() * 1000.0f);
		check_openxr(xrWaitFrame(xrSession, &frameWaitInfo, &frameState), TXT("xrWaitFrame"));
		output(TXT("tsWaitFrame sync %.3fms"), tsWaitFrame.get_time_since() * 1000.0f);
		tsWaitFrame.reset();

		// Begin frame immediately before GPU work
		XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
		check_openxr(xrBeginFrame(xrSession, &frameBeginInfo), TXT("xrBeginFrame"));
		output(TXT("tsBeginFrame sync %.3fms"), tsBeginFrame.get_time_since() * 1000.0f);
		tsBeginFrame.reset();

		Array<XrCompositionLayerBaseHeader*> layers;

		/*
		auto& rmd = renderModeData[renderMode];
		XrCompositionLayerProjection layerProj{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };

		if (frameState.shouldRender) {
			XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
			viewLocateInfo.displayTime = frameState.predictedDisplayTime;
			viewLocateInfo.space = xrPlaySpace;

			XrViewState viewState{ XR_TYPE_VIEW_STATE };
			uint32_t viewCountOutput;
			uint32_t viewCount = xrViews.get_size();
			check_openxr(xrLocateViews(xrSession, &viewLocateInfo, &viewState, viewCount, &viewCountOutput, xrViews.begin()), TXT("xrLocateViews"));

			// ...
			// Use viewState and frameState for scene render, and fill in projViews[2]
			// ...

			for_every(v, rmd.xrProjectionViews)
			{
				auto& xrView = xrViews[for_everys_index(v)];
				v->fov = xrView.fov;
				v->pose = xrView.pose;
			}

			// Assemble composition layers structure
			layerProj.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
			layerProj.space = xrPlaySpace;
			layerProj.viewCount = 2;
			layerProj.views = rmd.xrProjectionViews.begin();
			layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layerProj));
		}
		*/

		// End frame and submit layers, even if layers is empty due to shouldRender = false
		XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
		frameEndInfo.displayTime = frameState.predictedDisplayTime;
		frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		frameEndInfo.layerCount = (uint32_t)layers.get_size();
		frameEndInfo.layers = layers.get_data();
		check_openxr(xrEndFrame(xrSession, &frameEndInfo), TXT("xrEndFrame"));
	}
}
#endif
