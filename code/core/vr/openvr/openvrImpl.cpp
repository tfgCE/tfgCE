#include "openvrImpl.h"

#ifdef AN_WITH_OPENVR
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
#include "..\..\system\video\video3dPrimitives.h"

#include "..\..\containers\arrayStack.h"

#include <openvr.h>

//

using namespace VR;

//

//#ifdef AN_DEVELOPMENT
#define DEBUG_DECIDE_HANDS
//#endif

//#define AN_OPEN_VR_EXTENDED_DEBUG

//

float choose_component(vr::VRControllerAxis_t const & _from, Input::Device::Axis const & _axis)
{
	if (_axis.component == Input::Device::Axis::Y)
	{
		return _from.y;
	}
	else
	{
		return _from.x;
	}
}

//

String get_string_tracked_device_property(vr::IVRSystem* _openvrSystem, int _device, vr::ETrackedDeviceProperty _property)
{
	if (!_openvrSystem)
	{
		return String::empty();
	}

#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("_openvrSystem->GetStringTrackedDeviceProperty -> bufferLength"));
#endif
	uint32 bufferLength = _openvrSystem->GetStringTrackedDeviceProperty(_device, _property, nullptr, 0);

	String result;

	if (bufferLength > 0)
	{
		char *buffer = new char[bufferLength];
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
		output(TXT("_openvrSystem->GetStringTrackedDeviceProperty -> "));
#endif
		bufferLength = _openvrSystem->GetStringTrackedDeviceProperty(_device, _property, buffer, bufferLength);
		result = String::from_char8(buffer);
		delete[] buffer;
	}

	return result;
}

static bool run_openvr(tchar const* _when, vr::EVRInputError _error)
{
	if (_error == vr::VRInputError_None)
	{
		return true;
	}
	else
	{
		error(TXT("openvr error (%i) on: %S"), _error, _when);
		return false;
	}
}


//

static tchar* vrModelPrefix = TXT("openvr__");

Vector3 get_vector_from(vr::HmdVector3_t const & _v)
{
	// openvr: x right y up -z forward
	// our: x right y forward z up
	return Vector3(_v.v[0], -_v.v[2], _v.v[1]);
}

Vector3 get_translation_from(vr::HmdMatrix34_t const & _m)
{
	// openvr: x right y up -z forward
	// our: x right y forward z up
	return Vector3(_m.m[0][3], -_m.m[2][3], _m.m[1][3]);
}

Matrix44 get_matrix_from(vr::HmdMatrix34_t const & _m)
{
	Matrix44 m( _m.m[0][0], -_m.m[2][0],  _m.m[1][0], 0.0,
			   -_m.m[0][2],  _m.m[2][2], -_m.m[1][2], 0.0,
			    _m.m[0][1], -_m.m[2][1],  _m.m[1][1], 0.0,
			    _m.m[0][3], -_m.m[2][3],  _m.m[1][3], 1.0f
			   );
	return m;
}

//

bool OpenVRImpl::is_available()
{
	return vr::VR_IsRuntimeInstalled() && vr::VR_IsHmdPresent();
}

String setup_binding_path(OUT_ String * _path = nullptr, OUT String * _dirPath = nullptr)
{
	String path;
	String dirPath;

	{
		TCHAR currentDirBuf[16384];
		GetCurrentDirectory(16384, currentDirBuf);
		dirPath = String::convert_from(currentDirBuf) + IO::get_directory_separator();
		dirPath += String::printf(TXT("%S%S%S"), IO::get_user_game_data_directory().to_char(), TXT("openvr"), IO::get_directory_separator());
		if (!IO::Dir::does_exist(dirPath))
		{
			IO::Dir::create(dirPath);
		}
		path = dirPath + TXT("openvrActions.json");
	}

	assign_optional_out_param(_path, path);
	assign_optional_out_param(_dirPath, dirPath);

	return path;
}

void OpenVRImpl::create_config_files()
{
	String path;
	String dirPath;

	setup_binding_path(&path, &dirPath);

	String oculusTouchSubPath(TXT("openvrActionsOculusTouch.json"));
	String knucklesSubPath(TXT("openvrActionsKnuckles.json"));
	String viveWandSubPath(TXT("openvrActionsViveWand.json"));
	String holographicSubPath(TXT("openvrActionsHolographic.json"));
	{
		IO::File file;
		file.create(path);
		file.set_type(IO::InterfaceType::Text);
		file.write_line(TXT("{"));
		file.write_line(TXT("  \"actions\": ["));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/leftHand\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"skeleton\","));
		file.write_line(TXT("      \"skeleton\": \"/skeleton/hand/left\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/rightHand\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"skeleton\","));
		file.write_line(TXT("      \"skeleton\": \"/skeleton/hand/right\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/menu\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"boolean\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/buttonA\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"boolean\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/buttonB\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"boolean\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/joystick\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"vector2\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/trackpad\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"vector2\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/trigger\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"vector1\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/grip\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"vector1\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/buttonTrigger\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"boolean\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/buttonGrip\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"boolean\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/buttonTrackpad\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"boolean\""));
		file.write_line(TXT("    },"));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main/in/touchTrackpad\","));
		file.write_line(TXT("      \"requirement\": \"optional\","));
		file.write_line(TXT("      \"type\": \"boolean\""));
		file.write_line(TXT("    }"));
		file.write_line(TXT("  ],"));
		file.write_line(TXT("  \"action_sets\": ["));
		file.write_line(TXT("    {"));
		file.write_line(TXT("      \"name\": \"/actions/main\","));
		file.write_line(TXT("      \"usage\": \"leftright\""));
		file.write_line(TXT("    }"));
		file.write_line(TXT("  ],"));
		file.write_line(TXT("  \"default_bindings\": ["));
		file.write_line(TXT("   {"));
		file.write_line(TXT("      \"controller_type\": \"oculus_touch\","));
		file.write_line(String::printf(TXT("      \"binding_url\": \"%S\""), oculusTouchSubPath.to_char()));
		file.write_line(TXT("   },"));
		file.write_line(TXT("   {"));
		file.write_line(TXT("      \"controller_type\": \"knuckles\","));
		file.write_line(String::printf(TXT("      \"binding_url\": \"%S\""), knucklesSubPath.to_char()));
		file.write_line(TXT("   },"));
		file.write_line(TXT("   {"));
		file.write_line(TXT("      \"controller_type\": \"vive_controller\","));
		file.write_line(String::printf(TXT("      \"binding_url\": \"%S\""), viveWandSubPath.to_char()));
		file.write_line(TXT("   },"));
		file.write_line(TXT("   {"));
		file.write_line(TXT("      \"controller_type\": \"holographic_controller\","));
		file.write_line(String::printf(TXT("      \"binding_url\": \"%S\""), holographicSubPath.to_char()));
		file.write_line(TXT("   }"));
		file.write_line(TXT("  ] "));
		file.write_line(TXT("}"));
		file.close();
	}

	/*
	 *	Remember to update action_manifest_version when doing any changes
	 */

	{
		IO::File file;
		file.create(dirPath + oculusTouchSubPath);
		file.set_type(IO::InterfaceType::Text);
		file.write_line(TXT("{"));
		file.write_line(TXT("   \"action_manifest_version\" : 1,"));
		file.write_line(TXT("   \"alias_info\" : {},"));
		file.write_line(TXT("   \"bindings\" : {"));
		file.write_line(TXT("		\"/actions/main\": {"));
		file.write_line(TXT("			\"sources\": ["));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/system\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/menu\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/menu\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/trigger\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/joystick\","));
		file.write_line(TXT("					\"mode\": \"joystick\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/grip\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/grip\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/x\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/y\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/trigger\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/grip\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/grip\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/joystick\","));
		file.write_line(TXT("					\"mode\": \"joystick\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/a\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/b\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				}"));
		file.write_line(TXT("			]"));
		file.write_line(TXT("		}"));
		file.write_line(TXT("	},"));
		file.write_line(TXT("   \"category\" : \"steamvr_input\","));
		file.write_line(TXT("   \"controller_type\" : \"oculus_touch\","));
		file.write_line(TXT("   \"description\" : \"\","));
		file.write_line(TXT("   \"name\" : \"Oculus Touch for TeaForGod\","));
		file.write_line(TXT("   \"options\" : {},"));
		file.write_line(TXT("   \"simulated_actions\" : []"));
		file.write_line(TXT("}"));
	}

	{
		IO::File file;
		file.create(dirPath + knucklesSubPath);
		file.set_type(IO::InterfaceType::Text);
		file.write_line(TXT("{"));
		file.write_line(TXT("   \"action_manifest_version\" : 1,"));
		file.write_line(TXT("   \"alias_info\" : {},"));
		file.write_line(TXT("   \"bindings\" : {"));
		file.write_line(TXT("		\"/actions/main\": {"));
		file.write_line(TXT("			\"sources\": ["));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/system\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/menu\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/menu\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/trigger\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/thumbstick\","));
		file.write_line(TXT("					\"mode\": \"joystick\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/grip\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/grip\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/a\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/b\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/trigger\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/grip\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/grip\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/thumbstick\","));
		file.write_line(TXT("					\"mode\": \"joystick\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/a\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/b\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttonb\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				}"));
		file.write_line(TXT("			],"));
		file.write_line(TXT("			\"skeleton\": ["));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/skeleton/left\","));
		file.write_line(TXT("					\"output\": \"/actions/main/in/lefthand\""));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/skeleton/right\","));
		file.write_line(TXT("					\"output\": \"/actions/main/in/righthand\""));
		file.write_line(TXT("				}"));
		file.write_line(TXT("			]"));
		file.write_line(TXT("		}"));
		file.write_line(TXT("	},"));
		file.write_line(TXT("   \"category\" : \"steamvr_input\","));
		file.write_line(TXT("   \"controller_type\" : \"knuckles\","));
		file.write_line(TXT("   \"description\" : \"\","));
		file.write_line(TXT("   \"name\" : \"Index Controller for TeaForGod\","));
		file.write_line(TXT("   \"options\" : {},"));
		file.write_line(TXT("   \"simulated_actions\" : []"));
		file.write_line(TXT("}"));
	}

	{
		IO::File file;
		file.create(dirPath + viveWandSubPath);
		file.set_type(IO::InterfaceType::Text);
		file.write_line(TXT("{"));
		file.write_line(TXT("   \"action_manifest_version\" : 1,"));
		file.write_line(TXT("   \"alias_info\" : {},"));
		file.write_line(TXT("   \"bindings\" : {"));
		file.write_line(TXT("   	\"/actions/main\": {"));
		file.write_line(TXT("   		\"sources\": ["));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/left/input/trigger\","));
		file.write_line(TXT("   				\"mode\": \"trigger\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"pull\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("   					},"));
		file.write_line(TXT("   					\"click\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttontrigger\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/left/input/joystick\","));
		file.write_line(TXT("   				\"mode\": \"joystick\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"position\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/left/input/grip\","));
		file.write_line(TXT("   				\"mode\": \"trigger\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"pull\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/grip\""));
		file.write_line(TXT("   					},"));
		file.write_line(TXT("   					\"click\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttongrip\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/right/input/trigger\","));
		file.write_line(TXT("   				\"mode\": \"trigger\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"pull\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("   					},"));
		file.write_line(TXT("   					\"click\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttontrigger\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/right/input/grip\","));
		file.write_line(TXT("   				\"mode\": \"trigger\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"pull\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/grip\""));
		file.write_line(TXT("   					},"));
		file.write_line(TXT("   					\"click\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttongrip\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/right/input/joystick\","));
		file.write_line(TXT("   				\"mode\": \"joystick\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"position\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/left/input/application_menu\","));
		file.write_line(TXT("   				\"mode\": \"button\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"click\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("   					},"));
		file.write_line(TXT("   					\"held\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("   			{"));
		file.write_line(TXT("   				\"path\": \"/user/hand/right/input/application_menu\","));
		file.write_line(TXT("   				\"mode\": \"button\","));
		file.write_line(TXT("   				\"inputs\": {"));
		file.write_line(TXT("   					\"click\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("   					},"));
		file.write_line(TXT("   					\"held\": {"));
		file.write_line(TXT("   						\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("   					}"));
		file.write_line(TXT("   				}"));
		file.write_line(TXT("   			},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/trackpad\","));
		file.write_line(TXT("					\"mode\": \"trackpad\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"touch\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/touchtrackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttontrackpad\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/trackpad\","));
		file.write_line(TXT("					\"mode\": \"trackpad\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"touch\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/touchtrackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttontrackpad\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				}"));
		file.write_line(TXT("   		]"));
		file.write_line(TXT("   	}"));
		file.write_line(TXT("	},"));
		file.write_line(TXT("   \"category\" : \"steamvr_input\","));
		file.write_line(TXT("   \"controller_type\" : \"vive_controller\","));
		file.write_line(TXT("   \"description\" : \"\","));
		file.write_line(TXT("   \"name\" : \"Vive Controller for TeaForGod\","));
		file.write_line(TXT("   \"options\" : {},"));
		file.write_line(TXT("   \"simulated_actions\" : []"));
		file.write_line(TXT("}"));
	}

	{
		IO::File file;
		file.create(dirPath + holographicSubPath);
		file.set_type(IO::InterfaceType::Text);
		file.write_line(TXT("{"));
		file.write_line(TXT("   \"action_manifest_version\" : 1,"));
		file.write_line(TXT("   \"alias_info\" : {},"));
		file.write_line(TXT("   \"bindings\" : {"));
		file.write_line(TXT("		\"/actions/main\": {"));
		file.write_line(TXT("			\"sources\": ["));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/system\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/menu\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/menu\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/trigger\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttontrigger\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/trackpad\","));
		file.write_line(TXT("					\"mode\": \"trackpad\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttontrackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"touch\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/touchtrackpad\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/grip\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttongrip\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttongrip\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/application_menu\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/left/input/joystick\","));
		file.write_line(TXT("					\"mode\": \"joystick\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/trackpad\","));
		file.write_line(TXT("					\"mode\": \"trackpad\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttontrackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"touch\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/touchtrackpad\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trackpad\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/trigger\","));
		file.write_line(TXT("					\"mode\": \"trigger\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttontrigger\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"pull\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/trigger\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/grip\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttongrip\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttongrip\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/application_menu\","));
		file.write_line(TXT("					\"mode\": \"button\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"click\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						},"));
		file.write_line(TXT("						\"held\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/buttona\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				},"));
		file.write_line(TXT("				{"));
		file.write_line(TXT("					\"path\": \"/user/hand/right/input/joystick\","));
		file.write_line(TXT("					\"mode\": \"joystick\","));
		file.write_line(TXT("					\"inputs\": {"));
		file.write_line(TXT("						\"position\": {"));
		file.write_line(TXT("							\"output\": \"/actions/main/in/joystick\""));
		file.write_line(TXT("						}"));
		file.write_line(TXT("					}"));
		file.write_line(TXT("				}"));
		file.write_line(TXT("			]"));
		file.write_line(TXT("		}"));
		file.write_line(TXT("	},"));
		file.write_line(TXT("   \"category\" : \"steamvr_input\","));
		file.write_line(TXT("   \"controller_type\" : \"holographic_controller\","));
		file.write_line(TXT("   \"description\" : \"\","));
		file.write_line(TXT("   \"name\" : \"Windows Mixed Reality Controller for TeaForGod\","));
		file.write_line(TXT("   \"options\" : {},"));
		file.write_line(TXT("   \"simulated_actions\" : []"));
		file.write_line(TXT("}"));
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	{
		output(TXT("check of input files"));
		Array<String> files;
		files.push_back(path);
		files.push_back(dirPath + oculusTouchSubPath);
		files.push_back(dirPath + knucklesSubPath);
		files.push_back(dirPath + viveWandSubPath);
		for_every(file, files)
		{
			output(TXT("content of \"%S\""), file->to_char());
			IO::File f;
			if (f.open(*file))
			{
				f.set_type(IO::InterfaceType::Text);
				List<String> lines;
				f.read_lines(lines);
				for_every(line, lines)
				{
					output(TXT("%04i : %S"), for_everys_index(line), line->to_char());
				}
			}
		}
	}
#endif
}

OpenVRImpl::OpenVRImpl(OpenVR* _owner)
: base(_owner)
{
}

void OpenVRImpl::init_impl()
{
	System::Core::access_system_tags().set_tag(Name(TXT("openvr")));
	Splash::Logos::add(TXT("openvr"), SPLASH_LOGO_IMPORTANT);

	output(TXT("initialising openvr"));
	vr::HmdError hmdError;
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("vr::VR_Init"));
#endif
	openvrSystem = vr::VR_Init(&hmdError, vr::EVRApplicationType::VRApplication_Scene);
	output(TXT("vr_init ok"));

	// get input
	init_openvr_input();
}

void OpenVRImpl::init_openvr_input()
{
	if (openvrInput)
	{
		return;
	}

	output(TXT("get input"));
	openvrInput = vr::VRInput();

	if (openvrInput)
	{
		String bindingPath = setup_binding_path();
		if (!IO::File::does_exist(bindingPath))
		{
			create_config_files();
		}
		run_openvr(TXT("manifset"), openvrInput->SetActionManifestPath(bindingPath.to_char8_array().get_data()));
	}
	else
	{
		error(TXT("no openvr input"));
	}
}

void OpenVRImpl::shutdown_impl()
{
	if (openvrSystem)
	{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
		output(TXT("vr::VR_Shutdown"));
#endif
		vr::VR_Shutdown();
	}
}

void OpenVRImpl::create_device_impl()
{
	output(TXT("create device"));

	// setup chaperone
	output(TXT("chaperone"));
	openvrChaperone = vr::VRChaperone();

	output(TXT("chaperone ok"));
	// vive
	preferredFullScreenResolution.x = 2160;
	preferredFullScreenResolution.y = 1200;

	{
		String modelNumber = get_string_tracked_device_property(openvrSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);
		wireless = String::does_contain_icase(modelNumber.to_char(), TXT("oculus")) && String::does_contain_icase(modelNumber.to_char(), TXT("quest"));
		todo_hack(TXT("for time being we have no way to read a proper boundary"));
		mayHaveInvalidBoundary = wireless;
	}
}

void OpenVRImpl::close_device_impl()
{
	// why is there init?
	//output(TXT("initialising openvr - what?"));
	//vr::HmdError hmdError;
	//openvrSystem = vr::VR_Init(&hmdError, vr::EVRApplicationType::VRApplication_Scene);
	//output(TXT("vr_init ok"));
}

bool OpenVRImpl::can_load_play_area_rect_impl()
{
	static bool triedBefore = false;
	bool firstTry = ! triedBefore;
	triedBefore = true;

	if (firstTry)
	{
		output(TXT("can load play area rect"));
	}
	if (!openvrChaperone)
	{
		if (firstTry)
		{
			output(TXT("no chaperone"));
		}
		todo_note(TXT("message why - no device?"));
		return false;
	}

	if (firstTry)
	{
		output(TXT("get calibration state"));
	}
	auto calibrationState = openvrChaperone->GetCalibrationState();
	if (calibrationState != vr::ChaperoneCalibrationState_OK)
	{
		// note: do not throw an error, we should try to load it
		//owner->add_error(String(TXT("Could not get info abour chaperone. Please check device and setup.")));
		if (firstTry)
		{
			output(TXT("not calibrated"));
		}
		//todo_note(TXT("message why"));
		boundaryUnavailable = true;
		return false;
	}

	boundaryUnavailable = false; // available

	{
		Vector3 halfForward;
		Vector3 halfRight;
		load_play_area_rect_internal(true, OUT_ halfForward, OUT_ halfRight);
		float forwardLength = halfForward.length() * 2.0f;
		float rightLength = halfRight.length() * 2.0f;
		if (forwardLength > MAX_VR_SIZE ||
			rightLength   > MAX_VR_SIZE ||
			forwardLength < 0.000001f ||
			rightLength   < 0.000001f)
		{
			owner->add_warning(String(TXT("Could not get proper boundary dimension. Please check device and setup.")));
			output(TXT("couldn't get proper boundary dimensions"));
			return false;
		}
	}

	return true;
}

void OpenVRImpl::load_play_area_rect_internal(bool _loadAnything, OUT_ Vector3& _halfForward, OUT_ Vector3& _halfRight) const
{
	scoped_call_stack_info(TXT("OpenVRImpl::load_play_area_rect_internal"));

	output(TXT("load play area rect"));

	output(TXT("check play rect"));
	vr::HmdQuad_t rect;
	openvrChaperone->GetPlayAreaRect(&rect);
	Vector3 corners[4];
	for_count(int, i, 4)
	{
		corners[i] = owner->get_map_vr_space().vector_to_local(get_vector_from(rect.vCorners[i]));
	}

	Vector3 centre = (corners[0] + corners[1] + corners[2] + corners[3]) / 4.0f;
	int forwardIndices[2] = { 0, 1 };
	int rightIndices[2] = { 0, 3 };
	{
		int wf = 0;
		int wr = 0;
		for_count(int, i, 4)
		{
			if (corners[i].y > 0.0f)
			{
				if (wf < 2)
				{
					forwardIndices[wf] = i;
					++wf;
				}
			}
			if (corners[i].x > 0.0f)
			{
				if (wr < 2)
				{
					rightIndices[wr] = i;
					++wr;
				}
			}
		}
	}
	an_assert(centre.length() < 0.01f);
	Vector3 centreForward = (corners[forwardIndices[0]] + corners[forwardIndices[1]]) / 2.0f;
	Vector3 centreRight = (corners[rightIndices[0]] + corners[rightIndices[1]]) / 2.0f;

	_halfForward = centreForward - centre;
	_halfRight = centreRight - centre;

	{
		Array<Vector2> boundaryPoints;
		if (auto* openvrChaperoneSetup = vr::VRChaperoneSetup())
		{
			uint32_t quadsCount;
			openvrChaperoneSetup->GetLiveCollisionBoundsInfo(nullptr, &quadsCount);
			if (quadsCount > 0)
			{
				Array<vr::HmdQuad_t> quads;
				quads.set_size(quadsCount);
				openvrChaperoneSetup->GetLiveCollisionBoundsInfo(quads.begin(), &quadsCount);
				for_every(q, quads)
				{
					// grab only the first point, with it we can build whole boundary
					int i = 0;
					Vector3 v = get_vector_from(q->vCorners[i]);
					boundaryPoints.push_back(v.to_vector2());
				}
			}
		}
		owner->set_raw_boundary(boundaryPoints);
	}
}

bool OpenVRImpl::load_play_area_rect_impl(bool _loadAnything)
{
	scoped_call_stack_info(TXT("OpenVRImpl::load_play_area_rect_impl"));
	output(TXT("load play area rect"));

	Vector3 halfForward;
	Vector3 halfRight;
	load_play_area_rect_internal(_loadAnything, OUT_ halfForward, OUT_ halfRight);

	owner->set_play_area_rect(halfForward, halfRight);

	owner->act_on_possibly_invalid_boundary();

	output(TXT("half right %.3f"), owner->get_play_area_rect_half_right().length());
	output(TXT("half forward %.3f"), owner->get_play_area_rect_half_forward().length());
	if (owner->get_play_area_rect_half_right().length() >= owner->get_play_area_rect_half_forward().length())
	{
		return true;
	}

	return false;
}

void OpenVRImpl::init_hand_controller_track_device(int _index)
{
	if (handControllerTrackDevices[_index].deviceID == NONE)
	{
		return;
	}

#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::init_hand_controller_track_device %i"), _index);
#endif
	String modelNumberName = get_string_tracked_device_property(openvrSystem, handControllerTrackDevices[_index].deviceID, vr::Prop_ModelNumber_String);
	output(TXT("model number : %S"), modelNumberName.to_char());
	String renderModelName = get_string_tracked_device_property(openvrSystem, handControllerTrackDevices[_index].deviceID, vr::Prop_RenderModelName_String);
	output(TXT("render model : %S"), renderModelName.to_char());

	if (!renderModelName.is_empty())
	{
		owner->set_model_name_for_hand(_index, Name(String::printf(TXT("%S%S"), vrModelPrefix, renderModelName.to_char())));
	}
}

void OpenVRImpl::close_hand_controller_track_device(int _index)
{
	owner->set_model_name_for_hand(_index, Name::invalid());
}

void OpenVRImpl::update_track_device_indices()
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
		handControllerTrackDevices[i].clear();
	}

	int handIndex = 0;
	Array<int> handled;
	for_count(int, i, vr::k_unMaxTrackedDeviceCount)
	{
		output(TXT("check device %i"), i);
		if (openvrSystem->IsTrackedDeviceConnected(i) &&
			!handled.does_contain(i))
		{
			String modelNumber = get_string_tracked_device_property(openvrSystem, i, vr::Prop_ModelNumber_String);
			output(TXT("connected \"%S\""), modelNumber.to_char());
			if (! should_be_ignored(modelNumber))
			{
				auto tdc = openvrSystem->GetTrackedDeviceClass(i);
				switch (tdc)
				{
				case vr::TrackedDeviceClass_Controller:
					output(TXT("controller"));
					if (handIndex < Hands::Count)
					{
						handControllerTrackDevices[handIndex].deviceID = i;
						init_hand_controller_track_device(handIndex);
						++handIndex;
						handled.push_back(i);
					}
					break;
				case vr::TrackedDeviceClass_GenericTracker: break;
				case vr::TrackedDeviceClass_HMD: break;
				case vr::TrackedDeviceClass_Invalid: break;
				case vr::TrackedDeviceClass_TrackingReference: break; // base stations?
				default: break;
				}
			}
		}
	}

	owner->mark_new_controllers();
}

void OpenVRImpl::store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, vr::TrackedDevicePose_t* _trackedDevicePoses)
{
	if (_trackedDevicePoses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		_poseSet.rawViewAsRead = get_matrix_from(_trackedDevicePoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking).to_transform().to_world(viewInHeadPoseSpace);
	}

	for_count(int, i, Hands::Count)
	{
		int iController = handControllerTrackDevices[i].deviceID;
		Hand::Type handType = handControllerTrackDevices[i].hand;
		_poseSet.hands[i].rawHandTrackingRootPlacementAsRead.clear();
		if (iController != NONE)
		{
			auto & poseAsRead = _poseSet.hands[i].rawPlacementAsRead;
			if (_trackedDevicePoses[iController].bPoseIsValid)
			{
				poseAsRead = get_matrix_from(_trackedDevicePoses[iController].mDeviceToAbsoluteTracking).to_transform();
			}
			else
			{
				poseAsRead.clear();
			}
			if (_controls)
			{
				auto& controls = _controls->hands[i];
				controls.trackpadTouchDelta = Vector2::zero; // reset delta, we will update it if possible
				vr::VRControllerState_t state;
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
				output(TXT("OpenVRImpl::store : openvrSystem->GetControllerState"));
#endif
				bool fingersReadSeparately = false;
				auto* device = VR::Input::Devices::get(controls.source);
				auto& handDeviceInfo = handDeviceInfos[i];
				if (device && ! device->openVRUseLegacyInput && openvrInput)
				{	
					controls.upsideDown.clear();
					controls.insideToHead.clear();
					controls.pointerPinch.clear();
					controls.middlePinch.clear();
					controls.ringPinch.clear();
					controls.pinkyPinch.clear();
					controls.trigger = 0.0f; // trigger is from trigger axis and trigger button
					controls.grip = 0.0f; // grip is from axis, button and fingers
					if (device->fingersFromSkeleton)
					{
						fingersReadSeparately = true;
						vr::VRSkeletalSummaryData_t summaryData;
						if (openvrInput->GetSkeletalSummaryData(handDeviceInfo.handSkeletonActionHandle, vr::VRSummaryType_FromDevice, &summaryData) == vr::VRInputError_None)
						{
							controls.thumb = summaryData.flFingerCurl[vr::VRFinger_Thumb];
							controls.pointer = summaryData.flFingerCurl[vr::VRFinger_Index];
							controls.middle = summaryData.flFingerCurl[vr::VRFinger_Middle];
							controls.ring = summaryData.flFingerCurl[vr::VRFinger_Ring];
							controls.pinky = summaryData.flFingerCurl[vr::VRFinger_Pinky];

							float grip = max(controls.middle.get(0.0f), controls.ring.get(0.0f));
							controls.grip = controls.grip.get() + grip;
						}
					}
					{
						vr::InputDigitalActionData_t digitalData;
						if (openvrInput->GetDigitalActionData(handDeviceInfo.menuActionHandle, &digitalData, sizeof(vr::InputDigitalActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.buttons.systemMenu = digitalData.bActive && digitalData.bState;
						}
						if (openvrInput->GetDigitalActionData(handDeviceInfo.buttonAActionHandle, &digitalData, sizeof(vr::InputDigitalActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.buttons.primary = digitalData.bActive && digitalData.bState;
						}
						if (openvrInput->GetDigitalActionData(handDeviceInfo.buttonBActionHandle, &digitalData, sizeof(vr::InputDigitalActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.buttons.secondary = digitalData.bActive && digitalData.bState;
						}
						if (openvrInput->GetDigitalActionData(handDeviceInfo.buttonTriggerActionHandle, &digitalData, sizeof(vr::InputDigitalActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.trigger = controls.trigger.get() + (digitalData.bActive && digitalData.bState ? 1.0f : 0.0f);
						}
						if (openvrInput->GetDigitalActionData(handDeviceInfo.buttonGripActionHandle, &digitalData, sizeof(vr::InputDigitalActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.grip = controls.grip.get() + (digitalData.bActive && digitalData.bState ? 1.0f : 0.0f);
						}
						if (openvrInput->GetDigitalActionData(handDeviceInfo.buttonTrackpadActionHandle, &digitalData, sizeof(vr::InputDigitalActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.buttons.trackpad = digitalData.bActive && digitalData.bState;
						}
						if (openvrInput->GetDigitalActionData(handDeviceInfo.touchTrackpadActionHandle, &digitalData, sizeof(vr::InputDigitalActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.buttons.trackpadTouch = digitalData.bActive && digitalData.bState;
						}
					}
					{
						vr::InputAnalogActionData_t analogData;
						if (openvrInput->GetAnalogActionData(handDeviceInfo.joystickActionHandle, &analogData, sizeof(vr::InputAnalogActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							Vector2 joystick = Vector2(analogData.x, analogData.y);
							controls.joystick = joystick;
							controls.buttons.joystickLeft = joystick.x < -VRControls::JOYSTICK_DIR_THRESHOLD;
							controls.buttons.joystickRight = joystick.x > VRControls::JOYSTICK_DIR_THRESHOLD;
							controls.buttons.joystickDown = joystick.y < -VRControls::JOYSTICK_DIR_THRESHOLD;
							controls.buttons.joystickUp = joystick.y > VRControls::JOYSTICK_DIR_THRESHOLD;
						}
						if (openvrInput->GetAnalogActionData(handDeviceInfo.trackpadActionHandle, &analogData, sizeof(vr::InputAnalogActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
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
						if (openvrInput->GetAnalogActionData(handDeviceInfo.triggerActionHandle, &analogData, sizeof(vr::InputAnalogActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
						{
							controls.trigger = controls.trigger.get() + analogData.x;
						}
						if (openvrInput->GetAnalogActionData(handDeviceInfo.gripActionHandle, &analogData, sizeof(vr::InputAnalogActionData_t), handDeviceInfo.inputHandle) == vr::VRInputError_None)
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
				else
				{
					if (openvrSystem->GetControllerState(iController, &state, sizeof(state)))
					{
						for_count(int, iAxis, vr::k_unControllerStateAxisCount)
						{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
							output(TXT("OpenVRImpl::store : openvrSystem->GetInt32TrackedDeviceProperty"));
#endif
							int prop = openvrSystem->GetInt32TrackedDeviceProperty(iController, (vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + iAxis));
							if (prop == vr::k_eControllerAxis_Trigger)
							{
								if (!fingersReadSeparately)
								{
									if (device)
									{
										if (iAxis == device->triggerAxis.axis)
										{
											controls.trigger = choose_component(state.rAxis[iAxis], device->triggerAxis.component);
											controls.buttons.trigger = controls.trigger.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
										}
										if (iAxis == device->gripAxis.axis)
										{
											controls.grip = choose_component(state.rAxis[iAxis], device->gripAxis.component);
											controls.buttons.grip = controls.grip.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
										}
										if (iAxis == device->thumbAxis.axis)
										{
											controls.thumb = choose_component(state.rAxis[iAxis], device->thumbAxis.component);
											controls.buttons.thumb = controls.thumb.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
										}
										if (iAxis == device->pointerAxis.axis)
										{
											controls.pointer = choose_component(state.rAxis[iAxis], device->pointerAxis.component);
											controls.buttons.pointer = controls.pointer.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
										}
										if (iAxis == device->middleAxis.axis)
										{
											controls.middle = choose_component(state.rAxis[iAxis], device->middleAxis.component);
											controls.buttons.middle = controls.middle.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
										}
										if (iAxis == device->ringAxis.axis)
										{
											controls.ring = choose_component(state.rAxis[iAxis], device->ringAxis.component);
											controls.buttons.ring = controls.ring.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
										}
										if (iAxis == device->pinkyAxis.axis)
										{
											controls.pinky = choose_component(state.rAxis[iAxis], device->pinkyAxis.component);
											controls.buttons.pinky = controls.pinky.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
										}
									}
									else
									{
										// assume there's just trigger
										controls.trigger = state.rAxis[iAxis].x;
										controls.buttons.trigger = controls.trigger.get() > VRControls::AXIS_TO_BUTTON_THRESHOLD;
									}
								}
							}
							else if (prop == vr::k_eControllerAxis_Joystick)
							{
								auto const& joystick = state.rAxis[iAxis];
								controls.joystick = Vector2(joystick.x, joystick.y);
								controls.buttons.joystickLeft = joystick.x < -VRControls::JOYSTICK_DIR_THRESHOLD;
								controls.buttons.joystickRight = joystick.x > VRControls::JOYSTICK_DIR_THRESHOLD;
								controls.buttons.joystickDown = joystick.y < -VRControls::JOYSTICK_DIR_THRESHOLD;
								controls.buttons.joystickUp = joystick.y > VRControls::JOYSTICK_DIR_THRESHOLD;
							}
							else if (prop == vr::k_eControllerAxis_TrackPad)
							{
								auto const& trackpad = state.rAxis[iAxis];
								controls.trackpad = Vector2(trackpad.x, trackpad.y);
								if (controls.buttons.trackpadTouch)
								{
									Vector2 newTrackpadTouch = Vector2(trackpad.x, trackpad.y);
									if (controls.prevButtons.trackpadTouch)
									{
										controls.trackpadTouchDelta = newTrackpadTouch - controls.trackpadTouch;
									}
									controls.trackpadTouch = Vector2(trackpad.x, trackpad.y);
								}
							}
						}
					}
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

void OpenVRImpl::advance_events()
{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::advance_events begin"));
#endif

	// will init only if not init so far
	init_openvr_input();

	// advance input
	if (mainActionSetHandle != 0 && openvrInput)
	{
		vr::VRActiveActionSet_t actionSet = { 0 };
		actionSet.ulActionSet = mainActionSetHandle;
		run_openvr(TXT("UpdateActionState"), openvrInput->UpdateActionState(&actionSet, sizeof(vr::VRActiveActionSet_t), 1));
	}

	auto & vrControls = owner->access_controls();

	// events
	vr::VREvent_t event;
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::advance_events : openvrSystem->PollNextEvent"));
#endif
	while (openvrSystem->PollNextEvent(&event, sizeof(event)))
	{
		switch (event.eventType)
		{
		case vr::VREvent_OverlayShown:				::System::Core::pause_vr(bit(1)); break;
		case vr::VREvent_OverlayHidden:				::System::Core::unpause_vr(bit(1)); break;
		case vr::VREvent_DashboardActivated:		::System::Core::pause_vr(bit(2)); break;
		case vr::VREvent_DashboardDeactivated:		::System::Core::unpause_vr(bit(2)); break;
		case vr::VREvent_TrackedDeviceActivated:
			output(TXT("tracked device activated"));
			{
				if (openvrSystem->GetTrackedDeviceClass(event.trackedDeviceIndex) == vr::TrackedDeviceClass_Controller)
				{
					String modelNumber = get_string_tracked_device_property(openvrSystem, event.trackedDeviceIndex, vr::Prop_ModelNumber_String);
					output(TXT("tracked device (controller) activated (%S)"), modelNumber.to_char());
					todo_hack(TXT("force gamepads to be ignored"));
					if (!should_be_ignored(modelNumber))
					{
						for_count(int, i, Hands::Count)
						{
							if (handControllerTrackDevices[i].deviceID == NONE)
							{
								handControllerTrackDevices[i].deviceID = event.trackedDeviceIndex;
								owner->access_controls().hands[i].reset();
								init_hand_controller_track_device(i);
								break;
							}
						}
					}
					owner->decide_hands();
				}
			}
			break;
		case vr::VREvent_TrackedDeviceDeactivated:
			output(TXT("tracked device deactivated"));
			{
				if (openvrSystem->GetTrackedDeviceClass(event.trackedDeviceIndex) == vr::TrackedDeviceClass_Controller)
				{
					String modelNumber = get_string_tracked_device_property(openvrSystem, event.trackedDeviceIndex, vr::Prop_ModelNumber_String);
					output(TXT("tracked device (controller) deactivated (%S)"), modelNumber.to_char());
					for_count(int, i, Hands::Count)
					{
						if (handControllerTrackDevices[i].deviceID == event.trackedDeviceIndex)
						{
							handControllerTrackDevices[i].deviceID = NONE;
							owner->access_controls().hands[i].reset();
							close_hand_controller_track_device(i);
						}
					}
					owner->decide_hands();
				}
			}
			break;
		case vr::VREvent_MouseMove:
			vrControls.mouseMovement.x += event.data.mouse.x;
			vrControls.mouseMovement.y += event.data.mouse.y;
			break;
		case vr::VREvent_MouseButtonUp:
		case vr::VREvent_MouseButtonDown:
		{
			int mouseButtonIdx = NONE;
			if (event.data.mouse.button & vr::VRMouseButton_Middle) mouseButtonIdx = 1;
			if (event.data.mouse.button & vr::VRMouseButton_Right) mouseButtonIdx = 2;
			if (event.data.mouse.button & vr::VRMouseButton_Left) mouseButtonIdx = 0;
			if (mouseButtonIdx != NONE)
			{
				vrControls.mouseButton[mouseButtonIdx] = event.eventType == vr::VREvent_MouseButtonDown;
			}
		}
		break;
		case vr::VREvent_ButtonTouch:
		case vr::VREvent_ButtonUntouch:
		{
			int handIdx = find_hand_by_tracked_device_index(event.trackedDeviceIndex);
			if (handIdx != NONE)
			{
				auto & hand = vrControls.hands[handIdx];
				if (auto * device = VR::Input::Devices::get(hand.source))
				{
					if (device->openVRUseLegacyInput)
					{
						bool touch = event.eventType == vr::VREvent_ButtonTouch;
						if (event.data.controller.button >= vr::k_EButton_Axis0 && event.data.controller.button <= vr::k_EButton_Axis4)
						{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
							output(TXT("OpenVRImpl::advance_events : VREvent_ButtonTouch* : openvrSystem->GetInt32TrackedDeviceProperty"));
#endif
							int prop = openvrSystem->GetInt32TrackedDeviceProperty(event.trackedDeviceIndex, (vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + event.data.controller.button - vr::k_EButton_Axis0));
							if (prop == vr::k_eControllerAxis_TrackPad)
							{
								hand.buttons.trackpadTouch = touch;
							}
						}
					}
				}
			}
		}
		break;
		case vr::VREvent_ButtonPress:
		case vr::VREvent_ButtonUnpress:
		{
			int handIdx = find_hand_by_tracked_device_index(event.trackedDeviceIndex);
			if (handIdx != NONE)
			{
				auto& hand = vrControls.hands[handIdx];
				if (auto * device = VR::Input::Devices::get(hand.source))
				{
					if (device->openVRUseLegacyInput)
					{
						bool press = event.eventType == vr::VREvent_ButtonPress;
						if (event.data.controller.button >= vr::k_EButton_Axis0 && event.data.controller.button <= vr::k_EButton_Axis4)
						{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
							output(TXT("OpenVRImpl::advance_events : VREvent_ButtonPress* : openvrSystem->GetInt32TrackedDeviceProperty"));
#endif
							int prop = openvrSystem->GetInt32TrackedDeviceProperty(event.trackedDeviceIndex, (vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + event.data.controller.button - vr::k_EButton_Axis0));
							if (prop == vr::k_eControllerAxis_TrackPad)
							{
								hand.buttons.trackpad = press;
								hand.trackpadDir = Vector2::zero;
								if (!press)
								{
									hand.buttons.trackpadDirCentre = false;
									hand.buttons.trackpadDirLeft = false;
									hand.buttons.trackpadDirRight = false;
									hand.buttons.trackpadDirDown = false;
									hand.buttons.trackpadDirUp = false;
								}
								else
								{
									if (hand.trackpadTouch.x > VRControls::TRACKPAD_DIR_THRESHOLD)
									{
										hand.buttons.trackpadDirRight = true;
										hand.trackpadDir.x += 1.0f;
									}
									if (hand.trackpadTouch.x < -VRControls::TRACKPAD_DIR_THRESHOLD)
									{
										hand.buttons.trackpadDirLeft = true;
										hand.trackpadDir.x -= 1.0f;
									}
									if (hand.trackpadTouch.y > VRControls::TRACKPAD_DIR_THRESHOLD)
									{
										hand.buttons.trackpadDirUp = true;
										hand.trackpadDir.y += 1.0f;
									}
									if (hand.trackpadTouch.y < -VRControls::TRACKPAD_DIR_THRESHOLD)
									{
										hand.buttons.trackpadDirDown = true;
										hand.trackpadDir.y -= 1.0f;
									}
									hand.buttons.trackpadDirCentre =
										!hand.buttons.trackpadDirLeft &&
										!hand.buttons.trackpadDirRight &&
										!hand.buttons.trackpadDirDown &&
										!hand.buttons.trackpadDirUp;
								}
							}
						}
						else
						{
							switch (event.data.controller.button)
							{
							case vr::k_EButton_Grip: hand.buttons.grip = press; break;
							case vr::k_EButton_ApplicationMenu: hand.buttons.secondary = press; break;
							case vr::k_EButton_A: hand.buttons.primary = press; break;
							case vr::k_EButton_DPad_Left: hand.buttons.dpadLeft = press; break;
							case vr::k_EButton_DPad_Right: hand.buttons.dpadRight = press; break;
							case vr::k_EButton_DPad_Down: hand.buttons.dpadDown = press; break;
							case vr::k_EButton_DPad_Up: hand.buttons.dpadUp = press; break;
							}
						}
					}
				}
			}
		}
		break;
		}
	}
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::advance_events end"));
#endif
}

void OpenVRImpl::advance_pulses()
{
	// haptic pulses
	{
		int handIdx = 0;
		while (auto* pulse = owner->get_pulse(handIdx))
		{
			float currentStrength = pulse->get_current_strength() * MainConfig::global().get_vr_haptic_feedback();
			if (currentStrength > 0.0f)
			{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
				output(TXT("OpenVRImpl::advance_pulses : openvrSystem->TriggerHapticPulse"));
#endif
				openvrSystem->TriggerHapticPulse(handControllerTrackDevices[handIdx].deviceID, 0 /* there is just one axis to apply pulse to */, (unsigned short)(currentStrength * 3999.0f));
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
				output(TXT("OpenVRImpl::advance_pulses : openvrSystem->TriggerHapticPulse ok"));
#endif
			}
			++handIdx;
		}
	}
}

void OpenVRImpl::advance_poses()
{
	// get poses
	vr::TrackedDevicePose_t trackedDevicePoses[vr::k_unMaxTrackedDeviceCount];

	float expectedFrameTime = owner->get_expected_frame_time();
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::advance_poses : openvrSystem->GetDeviceToAbsoluteTrackingPose"));
#endif
	openvrSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, expectedFrameTime + vsyncToPhotonsTime, trackedDevicePoses, vr::k_unMaxTrackedDeviceCount);
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::advance_poses : openvrSystem->GetDeviceToAbsoluteTrackingPose ok"));
#endif

	auto & vrControls = owner->access_controls();
	auto & poseSet = owner->access_controls_pose_set();

	store(poseSet, &vrControls, trackedDevicePoses);
}

void OpenVRImpl::create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget)
{
	// no need to close internal render targets as they are normal render targets closing when ref count reaches zero

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

		{
			::System::RenderTargetSetup rtEyeSetup = ::System::RenderTargetSetup(
				resolution,
				0);
			rtEyeSetup.copy_output_textures_from(_source);
			rtEyeSetup.force_mip_maps();
			rtEyeSetup.limit_mip_maps(1);
			RENDER_TARGET_CREATE_INFO(TXT("openvr, internal render target, eye %i"), eyeIdx);
			eyeNativeOutputRenderTargets[eyeIdx] = new ::System::RenderTarget(rtEyeSetup);
		}
	}
}

void OpenVRImpl::on_render_targets_created()
{
}

void OpenVRImpl::next_frame()
{
}

bool OpenVRImpl::begin_render(System::Video3D * _v3d)
{
	if (!is_ok())
	{
		return false;
	}
	// get poses to render - 
	vr::TrackedDevicePose_t trackedDevicePoses[vr::k_unMaxTrackedDeviceCount];
	openvrCompositor->WaitGetPoses(trackedDevicePoses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
	store(owner->access_render_pose_set(), nullptr, trackedDevicePoses);
	owner->store_last_valid_render_pose_set();
	return true;
}

static void setup_to_render_to_render_target(::System::Video3D* _v3d, ::System::RenderTarget* _rtOverride)
{
	_v3d->setup_for_2d_display();

	if (_rtOverride)
	{
		_v3d->set_viewport(VectorInt2::zero, _rtOverride->get_size());
	}
	else
	{
		_v3d->set_default_viewport();
	}
	_v3d->set_near_far_plane(0.02f, 100.0f);

	_v3d->set_2d_projection_matrix_ortho();
	_v3d->access_model_view_matrix_stack().clear();
}

#define output_vr_error_case(what) if (_error == vr::what) error(TXT("vr error \"%S\""), TXT(#what));

static void output_vr_error(vr::EVRCompositorError _error)
{
	if (_error == vr::VRCompositorError_None)
	{
		return;
	}
	output_vr_error_case(VRCompositorError_RequestFailed);
	output_vr_error_case(VRCompositorError_IncompatibleVersion);
	output_vr_error_case(VRCompositorError_DoNotHaveFocus);
	output_vr_error_case(VRCompositorError_InvalidTexture);
	output_vr_error_case(VRCompositorError_IsNotSceneApplication);
	output_vr_error_case(VRCompositorError_TextureIsOnWrongDevice);
	output_vr_error_case(VRCompositorError_TextureUsesUnsupportedFormat);
	output_vr_error_case(VRCompositorError_SharedTexturesNotSupported);
	output_vr_error_case(VRCompositorError_IndexOutOfRange);
	output_vr_error_case(VRCompositorError_AlreadySubmitted);
	output_vr_error_case(VRCompositorError_InvalidBounds);
}

bool OpenVRImpl::end_render(System::Video3D * _v3d, ::System::RenderTargetPtr* _eyeRenderTargets)
{
	if (!is_ok())
	{
		return false;
	}

	::System::RenderTargetPtr* useRTs[2];

	DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR

	vr::EVRCompositorError compositorError;

	// if resolutions are different, render _eyeRenderTarget to eyeNativeOutputRenderTarget and provide it as input for composer
	for_count(int, eyeIdx, 2)
	{
		useRTs[eyeIdx] = &_eyeRenderTargets[eyeIdx];
		if (eyeNativeOutputRenderTargets[eyeIdx]->get_size() != _eyeRenderTargets[eyeIdx]->get_size())
		{
			// force change to avoid jiggy output
			if (_eyeRenderTargets[eyeIdx]->get_setup().get_output_texture_definition(0).get_filtering_mag() == System::TextureFiltering::nearest ||
				_eyeRenderTargets[eyeIdx]->get_setup().get_output_texture_definition(0).get_filtering_min() == System::TextureFiltering::nearest)
			{
				_eyeRenderTargets[eyeIdx]->change_filtering(0, System::TextureFiltering::linear, System::TextureFiltering::linear);
			}
			eyeNativeOutputRenderTargets[eyeIdx]->bind();
			setup_to_render_to_render_target(_v3d, eyeNativeOutputRenderTargets[eyeIdx].get());

			_eyeRenderTargets[eyeIdx]->render(0, _v3d, Vector2::zero, eyeNativeOutputRenderTargets[eyeIdx]->get_size().to_vector2());
			useRTs[eyeIdx] = &eyeNativeOutputRenderTargets[eyeIdx];

			eyeNativeOutputRenderTargets[eyeIdx]->resolve();
		}
	}

	vr::Texture_t leftEyeTexture = { (void*)(pointerint)((*useRTs[Eye::Left])->get_data_texture_id(0)), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	compositorError = openvrCompositor->Submit(vr::Eye_Left, &leftEyeTexture);
	output_vr_error(compositorError);
	vr::Texture_t rightEyeTexture = { (void*)(pointerint)((*useRTs[Eye::Right])->get_data_texture_id(0)), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	compositorError = openvrCompositor->Submit(vr::Eye_Right, &rightEyeTexture);
	output_vr_error(compositorError);

	// in sample app this figures as a hack
	//DIRECT_GL_CALL_ glFinish(); AN_GL_CHECK_FOR_ERROR
	//DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR

	todo_note(TXT("for time being to allow displaying same thing on main screen"));

	return true;
}

void OpenVRImpl::setup_rendering_on_init_video(::System::Video3D* _v3d)
{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::setup_rendering_on_init_video begin"));
#endif
	an_assert(is_ok(), TXT("openvr should be working"));
	an_assert(!openvrCompositor);

	uint32 eyeWidth, eyeHeight;
	openvrSystem->GetRecommendedRenderTargetSize(&eyeWidth, &eyeHeight);

	for_count(int, eyeIdx, 2)
	{
		vr::EVREye vrEye = eyeIdx == Eye::Left ? vr::Eye_Left : vr::Eye_Right;

		// openvr has wrong order in the header - it seems it should be bottom first followed by up
		openvrSystem->GetProjectionRaw(vrEye, &renderInfo.fovPort[eyeIdx].left, &renderInfo.fovPort[eyeIdx].right, &renderInfo.fovPort[eyeIdx].down, &renderInfo.fovPort[eyeIdx].up);
		renderInfo.fovPort[eyeIdx].tan_to_deg();

		// store render info
		renderInfo.eyeResolution[eyeIdx].x = eyeWidth;
		renderInfo.eyeResolution[eyeIdx].y = eyeHeight;
		renderInfo.fov[eyeIdx] = renderInfo.fovPort[eyeIdx].get_fov();
		renderInfo.aspectRatio[eyeIdx] = aspect_ratio(renderInfo.eyeResolution[eyeIdx]);
		renderInfo.lensCentreOffset[eyeIdx] = renderInfo.fovPort[eyeIdx].get_lens_centre_offset();

		auto eyeTransform = openvrSystem->GetEyeToHeadTransform(vrEye);
		renderInfo.eyeOffset[eyeIdx] = get_matrix_from(eyeTransform).to_transform();
	}

	viewInHeadPoseSpace = Transform::lerp(0.5f, renderInfo.eyeOffset[0], renderInfo.eyeOffset[1]);

	for (int eyeIdx = 0; eyeIdx < 2; ++eyeIdx)
	{
		renderInfo.eyeOffset[eyeIdx] = viewInHeadPoseSpace.to_local(renderInfo.eyeOffset[eyeIdx]);
	}

	openvrCompositor = vr::VRCompositor();
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::setup_rendering_on_init_video end"));
#endif
	if (!openvrCompositor)
	{
		error(TXT("no compositor?"));
	}

	if (renderInfo.eyeOffset[Eye::Left].get_translation().x > 0.0f)
	{
		error_stop(TXT("why eyes are not on the right side?"));
	}

}

Meshes::Mesh3D* OpenVRImpl::load_mesh(Name const & _modelName) const
{
#ifdef AN_USE_VR_RENDER_MODELS
	String modelNameWithoutPrefix = _modelName.to_string().replace(String(vrModelPrefix), String::empty());
	vr::RenderModel_t *renderModel = nullptr;
	Array<char8> modelName = modelNameWithoutPrefix.to_char8_array();
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::load_mesh"));
#endif
	vr::EVRRenderModelError result;
	while (true)
	{
		result = vr::VRRenderModels()->LoadRenderModel_Async(modelName.get_data(), &renderModel);
		if (result != vr::VRRenderModelError_Loading)
		{
			break;
		}
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
		output(TXT("still loading"));
#endif
		::System::Core::sleep(0.001f);
	}
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::load_mesh done loading"));
#endif
	if (result != vr::VRRenderModelError_None)
	{
		// no such model, skip it
		return nullptr;
	}
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("there is a mesh"));
#endif

	::Meshes::Builders::IPU ipu;

	ipu.will_need_at_least_points(renderModel->unVertexCount);
	ipu.will_need_at_least_polygons(renderModel->unTriangleCount);

	for_count(int, i, renderModel->unVertexCount)
	{
		auto const & v = renderModel->rVertexData[i];
		ipu.add_point(owner->get_map_vr_space().vector_to_local(get_vector_from(v.vPosition)));
	}

	for_count(int, i, renderModel->unTriangleCount)
	{
		auto const & t0 = renderModel->rIndexData[i * 3 + 0];
		auto const & t1 = renderModel->rIndexData[i * 3 + 1];
		auto const & t2 = renderModel->rIndexData[i * 3 + 2];
		ipu.add_triangle(0.0f, t2, t1, t0); // just as we swap axes
	}

	System::IndexFormat indexFormat;
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	System::VertexFormat vertexFormat;
	vertexFormat.with_normal();
	vertexFormat.with_padding();
	vertexFormat.calculate_stride_and_offsets();

	Array<int8> elementsData;
	Array<int8> vertexData;
	ipu.prepare_data_as_is(vertexFormat, indexFormat, vertexData, elementsData);
	int vertexCount = vertexData.get_size() / vertexFormat.get_stride();
	int elementsCount = elementsData.get_size() / indexFormat.get_stride();

	Meshes::Mesh3D* mesh = new Meshes::Mesh3D();
	Meshes::Mesh3DPart* part = mesh->load_part_data(vertexData.get_data(), elementsData.get_data(), Meshes::Primitive::Triangle, vertexCount, elementsCount, vertexFormat, indexFormat);
	return mesh;
#else
	return nullptr;
#endif
}

DecideHandsResult::Type OpenVRImpl::decide_hands(OUT_ int& _leftHand, OUT_ int& _rightHand)
{
	if (!openvrSystem)
	{
		// we won't ever decide
		return DecideHandsResult::CantTellAtAll;
	}

	DecideHandsResult::Type result = DecideHandsResult::Decided;
#ifdef DEBUG_DECIDE_HANDS
	output(TXT("openvr : decide hands"));
#endif
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::decide_hands begin"));
#endif
	_leftHand = NONE;
	_rightHand = NONE;
	for_count(int, i, 2)
	{
		if (handControllerTrackDevices[i].deviceID != NONE)
		{
			String modelNumber = get_string_tracked_device_property(openvrSystem, handControllerTrackDevices[i].deviceID, vr::Prop_ModelNumber_String);
			auto role = openvrSystem->GetControllerRoleForTrackedDeviceIndex(handControllerTrackDevices[i].deviceID);
			if (String::does_contain_icase(modelNumber.to_char(), TXT("left")))
			{
#ifdef DEBUG_DECIDE_HANDS
				output(TXT("  hand %i is left (by model)"), i);
#endif
				_leftHand = i;
				handControllerTrackDevices[i].hand = Hand::Left;
			}
			else if (String::does_contain_icase(modelNumber.to_char(), TXT("right")))
			{
#ifdef DEBUG_DECIDE_HANDS
				output(TXT("  hand %i is right (by model)"), i);
#endif
				_rightHand = i;
				handControllerTrackDevices[i].hand = Hand::Right;
			}
			else if (role == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand)
			{
#ifdef DEBUG_DECIDE_HANDS
				output(TXT("  hand %i is left (by role)"), i);
#endif
				_leftHand = i;
				handControllerTrackDevices[i].hand = Hand::Left;
			}
			else if (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand)
			{
#ifdef DEBUG_DECIDE_HANDS
				output(TXT("  hand %i is right (by role)"), i);
#endif
				_rightHand = i;
				handControllerTrackDevices[i].hand = Hand::Right;
			}
			else
			{
#ifdef DEBUG_DECIDE_HANDS
				warn(TXT("  hand %i is without a role"), i);
#endif
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
	if (_leftHand == NONE || _rightHand == NONE || _leftHand == _rightHand)
	{
#ifdef DEBUG_DECIDE_HANDS
		warn(TXT("  hands have invalid roles l:%i r:%i"), _leftHand, _rightHand);
#endif
		result = DecideHandsResult::UnableToDecide;
	}
	if (result == DecideHandsResult::UnableToDecide &&
		(_leftHand != NONE || _rightHand != NONE))
	{
#ifdef DEBUG_DECIDE_HANDS
		warn(TXT("  we have something found, mark as partially decided to keep it l:%i r:%i"), _leftHand, _rightHand);
#endif
		result = DecideHandsResult::PartiallyDecided;
	}
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::decide_hands end"));
#endif
	return result;
}

void OpenVRImpl::force_bounds_rendering(bool _force)
{
	openvrChaperone->ForceBoundsVisible(_force);
}

float OpenVRImpl::calculate_ideal_expected_frame_time() const
{
	if (!is_ok())
	{
		return 1.0f / 90.0f; // fallback
	}
	float frequency = openvrSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
	if (frequency != 0.0f)
	{
		return 1.0f / frequency;
	}
	else
	{
		return 1.0f / 90.0f;
	}
}

float OpenVRImpl::update_expected_frame_time()
{
	if (!is_ok())
	{
		return 1.0f / 90.0f; // fallback
	}
	vsyncToPhotonsTime = openvrSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
	float frequency = openvrSystem->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
	if (frequency != 0.0f)
	{
		return 1.0f / frequency;
	}
	else
	{
		return 1.0f / 90.0f;
	}
}

void OpenVRImpl::update_hands_controls_source()
{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::update_hands_controls_source begin"));
#endif
	bool handsChanged = false;
	for_count(int, index, Hands::Count)
	{
		String trackingSystemName = get_string_tracked_device_property(openvrSystem, handControllerTrackDevices[index].deviceID, vr::Prop_TrackingSystemName_String);
		String modelNumber = get_string_tracked_device_property(openvrSystem, handControllerTrackDevices[index].deviceID, vr::Prop_ModelNumber_String);

		if (handDeviceInfos[index].trackingSystemName != trackingSystemName ||
			handDeviceInfos[index].modelNumber != modelNumber)
		{
			handsChanged = true;
			handDeviceInfos[index].trackingSystemName = trackingSystemName;
			handDeviceInfos[index].modelNumber = modelNumber;
		}
		owner->access_controls().hands[index].source = VR::Input::Devices::find(modelNumber, trackingSystemName, String::empty());
		handDeviceInfos[index].inputHandle = 0;
		handDeviceInfos[index].handSkeletonActionHandle = 0;
		handDeviceInfos[index].menuActionHandle = 0;
		handDeviceInfos[index].buttonAActionHandle = 0;
		handDeviceInfos[index].buttonBActionHandle = 0;
		handDeviceInfos[index].buttonTriggerActionHandle = 0;
		handDeviceInfos[index].buttonGripActionHandle = 0;
		handDeviceInfos[index].buttonTrackpadActionHandle = 0;
		handDeviceInfos[index].touchTrackpadActionHandle = 0;

		handDeviceInfos[index].joystickActionHandle = 0;
		handDeviceInfos[index].trackpadActionHandle = 0;
		handDeviceInfos[index].triggerActionHandle = 0;
		handDeviceInfos[index].gripActionHandle = 0;
		if (auto * device = VR::Input::Devices::get(owner->access_controls().hands[index].source))
		{
			if (! device->openVRUseLegacyInput && openvrInput)
			{
				bool isLeftHand = owner->get_left_hand() == index;
				run_openvr(TXT("inputHandle"), openvrInput->GetInputSourceHandle(isLeftHand ? "/user/hand/left" : "/user/hand/right", &handDeviceInfos[index].inputHandle));
				run_openvr(TXT("handSkeletonActionHandle"), openvrInput->GetActionHandle(isLeftHand ? "/actions/main/in/leftHand" : "/actions/main/in/rightHand", &handDeviceInfos[index].handSkeletonActionHandle));
				run_openvr(TXT("menuActionHandle"), openvrInput->GetActionHandle("/actions/main/in/menu", &handDeviceInfos[index].menuActionHandle));
				run_openvr(TXT("buttonAActionHandle"), openvrInput->GetActionHandle("/actions/main/in/buttonA", &handDeviceInfos[index].buttonAActionHandle));
				run_openvr(TXT("buttonBActionHandle"), openvrInput->GetActionHandle("/actions/main/in/buttonB", &handDeviceInfos[index].buttonBActionHandle));
				run_openvr(TXT("buttonTriggerActionHandle"), openvrInput->GetActionHandle("/actions/main/in/buttonTrigger", &handDeviceInfos[index].buttonTriggerActionHandle));
				run_openvr(TXT("buttonGripActionHandle"), openvrInput->GetActionHandle("/actions/main/in/buttonGrip", &handDeviceInfos[index].buttonGripActionHandle));
				run_openvr(TXT("buttonTrackpadActionHandle"), openvrInput->GetActionHandle("/actions/main/in/buttonTrackpad", &handDeviceInfos[index].buttonTrackpadActionHandle));
				run_openvr(TXT("touchTrackpadActionHandle"), openvrInput->GetActionHandle("/actions/main/in/touchTrackpad", &handDeviceInfos[index].touchTrackpadActionHandle));
				run_openvr(TXT("joystickActionHandle"), openvrInput->GetActionHandle("/actions/main/in/joystick", &handDeviceInfos[index].joystickActionHandle));
				run_openvr(TXT("trackpadActionHandle"), openvrInput->GetActionHandle("/actions/main/in/trackpad", &handDeviceInfos[index].trackpadActionHandle));
				run_openvr(TXT("triggerActionHandle"), openvrInput->GetActionHandle("/actions/main/in/trigger", &handDeviceInfos[index].triggerActionHandle));
				run_openvr(TXT("gripActionHandle"), openvrInput->GetActionHandle("/actions/main/in/grip", &handDeviceInfos[index].gripActionHandle));
			}
		}
	}
	{
		mainActionSetHandle = 0;
		if (auto * device = VR::Input::Devices::get(owner->access_controls().hands[0].source))
		{
			if (!device->openVRUseLegacyInput && openvrInput)
			{
				run_openvr(TXT("GetActionSetHandle"), openvrInput->GetActionSetHandle("/actions/main", &mainActionSetHandle));
				activeActionSet.nPriority = 0;
				activeActionSet.ulActionSet = mainActionSetHandle;
				activeActionSet.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
				activeActionSet.ulSecondaryActionSet = vr::k_ulInvalidActionSetHandle;
			}
		}
	}
	if (handsChanged)
	{
		output(TXT("openvr : hands"));
		output(TXT("    : tracking system name : model number"));
		for_count(int, index, Hands::Count)
		{
			output(TXT("  %i : %c : dev=%i : inputhandle=%i : %S : %S"),
				index,
				owner->get_left_hand() == index? 'l' : (owner->get_right_hand() == index? 'r' : ' '),
				handControllerTrackDevices[index].deviceID,
				handDeviceInfos[index].inputHandle,
				handDeviceInfos[index].trackingSystemName.to_char(),
				handDeviceInfos[index].modelNumber.to_char());
		}
	}
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::update_hands_controls_source end"));
#endif
}

void OpenVRImpl::draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font)
{
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::draw_debug_controls begin"));
#endif
	Vector2 at = _at;
	Vector2 triggerSize = Vector2(_width, _width * 0.1f);
	Vector2 padSize = Vector2(_width, _width);
	Vector2 separation = Vector2(_width * 0.05f, _width * 0.05f);

	Vector2 buttonSize= Vector2(_width / 8.0f, _width / 8.0f);

	int iController = handControllerTrackDevices[_hand].deviceID;
	if (iController != NONE)
	{
		vr::VRControllerState_t state;
		if (openvrSystem->GetControllerState(iController, &state, sizeof(state)))
		{
			for_count(int, iAxis, vr::k_unControllerStateAxisCount)
			{
				int prop = openvrSystem->GetInt32TrackedDeviceProperty(iController, (vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + iAxis));
				if (prop == vr::k_eControllerAxis_Trigger)
				{
					at.y -= triggerSize.y * 0.5f;
					::System::Video3DPrimitives::fill_rect_2d(Colour::blue.with_alpha(0.3f), at - triggerSize * 0.5f, at + triggerSize * 0.5f);
					::System::Video3DPrimitives::fill_rect_2d(Colour::red.with_alpha(0.8f), at - triggerSize * 0.5f, Vector2(at.x - triggerSize.x * 0.5f + triggerSize.x * state.rAxis[iAxis].x, at.y + triggerSize.y * 0.5f));
					_font->draw_text_at(::System::Video3D::get(), String::printf(TXT("%i"), iAxis).to_char(), Colour::white, at, Vector2::one, Vector2::half, false);
					at.y -= triggerSize.y;
					::System::Video3DPrimitives::fill_rect_2d(Colour::blue.with_alpha(0.3f), at - triggerSize * 0.5f, at + triggerSize * 0.5f);
					::System::Video3DPrimitives::fill_rect_2d(Colour::red.with_alpha(0.8f), at - triggerSize * 0.5f, Vector2(at.x - triggerSize.x * 0.5f + triggerSize.x * state.rAxis[iAxis].y, at.y + triggerSize.y * 0.5f));
					at.y -= triggerSize.y;
					at.y += triggerSize.y * 0.5f;
				}
				else if (prop == vr::k_eControllerAxis_Joystick ||
					prop == vr::k_eControllerAxis_TrackPad)
				{
					at.y -= padSize.y * 0.5f;
					::System::Video3DPrimitives::fill_rect_2d(Colour::blue.with_alpha(0.3f), at - padSize * 0.5f, at + padSize * 0.5f);
					auto const & joystick = state.rAxis[iAxis];
					Vector2 joyPt = Vector2(joystick.x, joystick.y);
					::System::Video3DPrimitives::fill_rect_2d(Colour::red.with_alpha(0.8f), at + joyPt * 0.48f * padSize - padSize * 0.02f, at + joyPt * 0.48f * padSize + padSize * 0.02f);
					_font->draw_text_at(::System::Video3D::get(), String::printf(TXT("%i"), iAxis).to_char(), Colour::white, at, Vector2::one, Vector2::half, false);
					at.y -= padSize.y;
					at.y += padSize.y * 0.5f;
				}
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
					if ((state.ulButtonTouched >> iButton) & 1)
					{
						::System::Video3DPrimitives::fill_rect_2d(Colour::orange.with_alpha(0.3f), a - buttonSize * 0.5f, a + buttonSize * 0.5f);
					}
					if ((state.ulButtonPressed >> iButton) & 1)
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
#ifdef AN_OPEN_VR_EXTENDED_DEBUG
	output(TXT("OpenVRImpl::draw_debug_controls end"));
#endif
}

void OpenVRImpl::update_render_scaling(REF_ float & _scale)
{
	// leave it as it is?
}

void OpenVRImpl::set_render_mode(RenderMode::Type _mode)
{
	todo_note(TXT("implement various modes"));
}

bool OpenVRImpl::should_be_ignored(String const& _modelNumber)
{
	if (_modelNumber == TXT("gamepad")) return true;
	if (String::does_contain_icase(_modelNumber.to_char(), TXT("gamepad"))) return true;
	if (String::does_contain_icase(_modelNumber.to_char(), TXT("treadmill"))) return true;
	return false;
}

#endif
