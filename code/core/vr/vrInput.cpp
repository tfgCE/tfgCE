#include "vrInput.h"

#include "iVR.h"

#include "..\system\input.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define LOG_FIND_DEVICE
//#define LOG_FIND_DEVICE_ELABORATE

//

using namespace VR;

//

bool Input::Device::Axis::load_from_xml(IO::XML::Node const * _node, tchar const * axisAttr, tchar const * componentAttr)
{
	bool result = true;

	axis = _node->get_int_attribute(axisAttr, axis);
	String componentValue = _node->get_string_attribute(componentAttr, component == X ? String(TXT("x")) : String(TXT("y")));
	if (String::compare_icase(componentValue.to_char(), TXT("x")))
	{
		component = X;
	}
	else if (String::compare_icase(componentValue.to_char(), TXT("y")))
	{
		component = Y;
	}
	else
	{
		component = X;
	}

	return result;
}

bool Input::Device::Axis::save_to_xml(IO::XML::Node * _node, tchar const * axisAttr, tchar const * componentAttr)
{
	bool result = true;

	if (axis != NONE)
	{
		_node->set_int_attribute(axisAttr, axis);
		if (component != X)
		{
			_node->set_attribute(componentAttr, component == X ? String(TXT("x")) : String(TXT("y")));
		}
	}

	return result;
}

//

bool Input::Device::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	forPromptName = _node->get_name_attribute(TXT("forPromptName"), forPromptName);

	force = _node->get_bool_attribute_or_from_child_presence(TXT("force"), force);

	for_every(node, _node->children_named(TXT("forcedForVRSystem")))
	{
		forcedForVRSystems.push_back(Name(node->get_text()));
	}
	
	for_every(node, _node->children_named(TXT("modelNumber")))
	{
		modelNumbers.push_back(node->get_text());
	}

	for_every(node, _node->children_named(TXT("trackingSystemName")))
	{
		trackingSystemNames.push_back(node->get_text());
	}

	for_every(node, _node->children_named(TXT("controllerName")))
	{
		controllerNames.push_back(node->get_text());
	}

	inputTags.load_from_xml_attribute_or_child_node(_node, TXT("inputTags"));

	result &= triggerAxis.load_from_xml(_node, TXT("triggerAxis"), TXT("triggerAxisComponent"));
	result &= gripAxis.load_from_xml(_node, TXT("gripAxis"), TXT("gripAxisComponent"));

	result &= thumbAxis.load_from_xml(_node, TXT("thumbAxis"), TXT("thumbAxisComponent"));
	result &= pointerAxis.load_from_xml(_node, TXT("pointerAxis"), TXT("pointerAxisComponent"));
	result &= middleAxis.load_from_xml(_node, TXT("middleAxis"), TXT("middleAxisComponent"));
	result &= ringAxis.load_from_xml(_node, TXT("ringAxis"), TXT("ringAxisComponent"));
	result &= pinkyAxis.load_from_xml(_node, TXT("pinkyAxis"), TXT("pinkyAxisComponent"));

	openVRUseLegacyInput = _node->get_bool_attribute_or_from_child_presence(TXT("openVRUseLegacyInput"), openVRUseLegacyInput);
	fingersFromSkeleton = _node->get_bool_attribute_or_from_child_presence(TXT("fingersFromSkeleton"), fingersFromSkeleton);

	return result;
}

bool Input::Device::save_to_xml(IO::XML::Node * _node)
{
	bool result = true;

	if (IO::XML::Node* node = _node->add_node(TXT("device")))
	{
		node->set_attribute(TXT("name"), name.to_string());

		if (forPromptName.is_valid())
		{
			node->set_attribute(TXT("forPromptName"), forPromptName.to_string());
		}

		for_every(forcedForVRSystem, forcedForVRSystems)
		{
			if (auto* n = node->add_node(TXT("forcedForVRSystem")))
			{
				n->add_text(forcedForVRSystem->to_string());
			}
		}
		
		for_every(modelNumber, modelNumbers)
		{
			if (auto* n = node->add_node(TXT("modelNumber")))
			{
				n->add_text(*modelNumber);
			}
		}

		for_every(trackingSystemName, trackingSystemNames)
		{
			if (auto* n = node->add_node(TXT("trackingSystemName")))
			{
				n->add_text(*trackingSystemName);
			}
		}

		for_every(controllerName, controllerNames)
		{
			if (auto* n = node->add_node(TXT("controllerName")))
			{
				n->add_text(*controllerName);
			}
		}

		if (!inputTags.is_empty())
		{
			node->set_attribute(TXT("inputTags"), inputTags.to_string());
		}

		result &= triggerAxis.save_to_xml(node, TXT("triggerAxis"), TXT("triggerAxisComponent"));
		result &= gripAxis.save_to_xml(node, TXT("gripAxis"), TXT("gripAxisComponent"));

		result &= thumbAxis.save_to_xml(node, TXT("thumbAxis"), TXT("thumbAxisComponent"));
		result &= pointerAxis.save_to_xml(node, TXT("pointerAxis"), TXT("pointerAxisComponent"));
		result &= middleAxis.save_to_xml(node, TXT("middleAxis"), TXT("middleAxisComponent"));
		result &= ringAxis.save_to_xml(node, TXT("ringAxis"), TXT("ringAxisComponent"));
		result &= pinkyAxis.save_to_xml(node, TXT("pinkyAxis"), TXT("pinkyAxisComponent"));

		if (openVRUseLegacyInput)
		{
			node->set_bool_attribute(TXT("openVRUseLegacyInput"), openVRUseLegacyInput);
		}
		if (fingersFromSkeleton)
		{
			node->set_bool_attribute(TXT("fingersFromSkeleton"), fingersFromSkeleton);
		}
	}

	return result;
}

//

bool Input::DeviceGroupName::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	forPromptName = _node->get_name_attribute(TXT("forPromptName"), forPromptName);

	for_every(node, _node->children_named(TXT("device")))
	{
		Name devName = node->get_name_attribute(TXT("name"));
		if (devName.is_valid())
		{
			devices.push_back(devName);
		}
	}

	return result;
}

bool Input::DeviceGroupName::save_to_xml(IO::XML::Node * _node)
{
	bool result = true;

	if (IO::XML::Node* node = _node->add_node(TXT("deviceGroup")))
	{
		node->set_attribute(TXT("name"), name.to_string());

		if (forPromptName.is_valid())
		{
			node->set_attribute(TXT("forPromptName"), forPromptName.to_string());
		}

		for_every(devName, devices)
		{
			if (auto* n = node->add_node(TXT("device")))
			{
				n->set_attribute(TXT("name"), devName->to_string());
			}
		}
	}

	return result;
}

//

Input::Devices* Input::Devices::s_devices = nullptr;

void Input::Devices::initialise_static()
{
	an_assert(!s_devices);
	s_devices = new Devices();
}

void Input::Devices::close_static()
{
	an_assert(s_devices);
	delete_and_clear(s_devices);
}

bool Input::Devices::load_from_xml(IO::XML::Node const * _node)
{
	an_assert(s_devices);

	bool result = true;

	for_every(node, _node->children_named(TXT("device")))
	{
		Device dev;
		if (dev.load_from_xml(node))
		{
			int idx = s_devices->devices.get_size();
			for_every(device, s_devices->devices)
			{
				if (device->name == dev.name)
				{
					idx = for_everys_index(device);
					break;
				}
			}

			dev.id = 1 << idx;
			s_devices->devices.set_size(max(idx + 1, s_devices->devices.get_size()));
			s_devices->devices[idx] = dev;

			// pretend vr input should be loaded at this time
			if (MainConfig::global().get_pretend_vr_input_system().is_valid() &&
				dev.name == MainConfig::global().get_pretend_vr_input_system())
			{
				::System::Input::get()->set_usage_tags_from(dev.inputTags);
			}
		}
	}

	for_every(node, _node->children_named(TXT("deviceGroup")))
	{
		DeviceGroupName dgn;
		if (dgn.load_from_xml(node))
		{
			for_every(d, dgn.devices)
			{
				String devName = d->to_string();
				auto did = Devices::parse(devName, true);
				dgn.id |= did;
			}

			s_devices->groupNames.push_back(dgn);
		}
	}

	return result;
}

bool Input::Devices::save_to_xml(IO::XML::Node * _node)
{
	an_assert(s_devices);

	bool result = true;

	if (IO::XML::Node* node = _node->add_node(TXT("vrInputDevices")))
	{
		for_every(device, s_devices->devices)
		{
			result &= device->save_to_xml(node);
		}
		for_every(dgn, s_devices->groupNames)
		{
			result &= dgn->save_to_xml(node);
		}
	}

	return result;
}

Array<Input::Device> const & Input::Devices::get_all()
{
	return s_devices->devices;
}

Input::Device const* Input::Devices::get(DeviceID _id)
{
	for_every(device, s_devices->devices)
	{
		if (device->id == _id)
		{
			return device;
		}
	}

	return nullptr;
}

Input::DeviceID Input::Devices::find(String const & _modelNumber, String const & _trackingSystemName, String const& _controllerName)
{
	an_assert(s_devices);

#ifdef LOG_FIND_DEVICE
	output(TXT("[vr-input] find vr input device"));
	output(TXT("[vr-input]   model=\"%S\""), _modelNumber.to_char());
	output(TXT("[vr-input]   tracking system=\"%S\""), _trackingSystemName.to_char());
	output(TXT("[vr-input]   controller=\"%S\""), _controllerName.to_char());
#endif

	// check if forced first
	for_every(device, s_devices->devices)
	{
		if (device->force)
		{
#ifdef LOG_FIND_DEVICE
			output(TXT("[vr-input]   use forced: %S"), device->name.to_char());
#endif
			return device->id;
		}
	}

	// check if forced for vr system
	Name vrSystemName;
	if (auto* vr = IVR::get())
	{
		vrSystemName = vr->get_name();
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (MainConfig::global().get_pretend_vr_input_system().is_valid())
	{
		vrSystemName = MainConfig::global().get_pretend_vr_input_system();
	}
#endif

#ifdef LOG_FIND_DEVICE
	output(TXT("[vr-input]   vr system name: \"%S\""), vrSystemName.to_char());
#endif

	for_every(device, s_devices->devices)
	{
		if (device->forcedForVRSystems.does_contain(vrSystemName))
		{
#ifdef LOG_FIND_DEVICE
			output(TXT("[vr-input]   use forced for vr system: \"%S\""), device->name.to_char());
#endif
			return device->id;
		}
	}

	if (_trackingSystemName.is_empty() && _modelNumber.is_empty() && _controllerName.is_empty())
	{
#ifdef LOG_FIND_DEVICE
		output(TXT("[vr-input]   none provided, use any"));
#endif
		// not plugged in? pretend it's anything, when we connect a device, we will set it up properly
		return Devices::all;
	}

	// get to lower
	String modelNumber = _modelNumber.to_lower();
	String trackingSystemName = _trackingSystemName.to_lower();
	String controllerName = _controllerName.to_lower();

	// order:
	//  by controller name
	//	by pair (model + tracking system)
	//  by model number alone
	//  by tracking system alone
	// more default tracking systems should be sooner/earlier
	for(int tryIdx = 0; tryIdx < 4; ++ tryIdx)
	{
		bool checkModelNumber = tryIdx == 1 || tryIdx == 2;
		bool checkTrackingSystem = tryIdx == 1 || tryIdx == 3;
		bool checkController = tryIdx == 0;
#ifdef LOG_FIND_DEVICE_ELABORATE
		output(TXT("[vr-input]  pass index %i model:%c tracksys:%c controller:%c"), tryIdx, checkModelNumber ? 'C' :'-', checkTrackingSystem ? 'C' :'-', checkController ? 'C' :'-');
#endif
		{	// maybe there's no point in checking?
			int reqCount = 0;
			int provReqCount = 0;
			reqCount += checkModelNumber ? 1 : 0;
			provReqCount += (checkModelNumber && !modelNumber.is_empty()) ? 1 : 0;
			reqCount += checkTrackingSystem ? 1 : 0;
			provReqCount += (checkTrackingSystem && !trackingSystemName.is_empty()) ? 1 : 0;
			reqCount += checkController ? 1 : 0;
			provReqCount += (checkController && !controllerName.is_empty()) ? 1 : 0;
			if (reqCount > 0 && provReqCount < reqCount)
			{
#ifdef LOG_FIND_DEVICE_ELABORATE
				output(TXT("[vr-input]    skip pass"));
#endif
				continue;
			}
		}
		for_every(device, s_devices->devices)
		{
#ifdef LOG_FIND_DEVICE_ELABORATE
			output(TXT("[vr-input]   check device \"%S\""), device->name.to_char());
#endif
			bool modelNumberOk = false;
			for_every(mn, device->modelNumbers)
			{
#ifdef LOG_FIND_DEVICE_ELABORATE
				output(TXT("[vr-input]     model number: \"%S\""), mn->to_char());
#endif
				String mnl = mn->to_lower();
				if (String::does_contain_icase(modelNumber.to_char(), mnl.to_char()))
				{
#ifdef LOG_FIND_DEVICE_ELABORATE
					output(TXT("[vr-input]       MATCH"));
#endif
					modelNumberOk = true;
					break;
				}
			}
			bool trackingSystemOk = false;
			for_every(tsn, device->trackingSystemNames)
			{
#ifdef LOG_FIND_DEVICE_ELABORATE
				output(TXT("[vr-input]     tracking system: \"%S\""), tsn->to_char());
#endif
				String tsnl = tsn->to_lower();
				if (String::does_contain_icase(trackingSystemName.to_char(), tsnl.to_char()))
				{
#ifdef LOG_FIND_DEVICE_ELABORATE
					output(TXT("[vr-input]       MATCH"));
#endif
					trackingSystemOk = true;
					break;
				}
			}
			bool controllerOk = false;
			for_every(tsn, device->controllerNames)
			{
#ifdef LOG_FIND_DEVICE_ELABORATE
				output(TXT("[vr-input]     controller name: \"%S\""), tsn->to_char());
#endif
				String tsnl = tsn->to_lower();
				if (String::does_contain_icase(controllerName.to_char(), tsnl.to_char()))
				{
#ifdef LOG_FIND_DEVICE_ELABORATE
					output(TXT("[vr-input]       MATCH"));
#endif
					controllerOk = true;
					break;
				}
			}

			if ((checkModelNumber && !modelNumberOk) ||
				(checkTrackingSystem && !trackingSystemOk) ||
				(checkController && !controllerOk))
			{
				// does not match
				continue;
			}
			// found!
			output(TXT("[vr-input] vr input device: %S"), device->name.to_char());
			if (device->openVRUseLegacyInput)
			{
				output(TXT("[vr-input]   uses legacy input (open vr)"));
			}
			return device->id;
		}
	}

	if (! s_devices->devices.is_empty())
	{
		warn(TXT("not recognised tracking devices \"%S\":\"%S\", controller:\"%S\", assuming \"%S\" (first on the list)"), _trackingSystemName.to_char(), _modelNumber.to_char(), _controllerName.to_char(), s_devices->devices.get_first().name.to_char());
		return s_devices->devices.get_first().id;
	}

	warn(TXT("not recognised tracking devices \"%S\":\"%S\", controller:\"%S\", no devices defined"), _trackingSystemName.to_char(), _modelNumber.to_char(), _controllerName.to_char());

	return Devices::all;
}

Input::DeviceID Input::Devices::parse(REF_ String & _inputString, Optional<bool> const& _onlyDevices)
{
	an_assert(s_devices);

	Input::DeviceID result = 0;

	for_every(device, s_devices->devices)
	{
		if (String::does_contain(_inputString.to_char(), device->name.to_char()))
		{
			_inputString = _inputString.replace(device->name.to_string(), String::empty()).trim();
			result |= device->id;
		}
	}

	for_every(dgn, s_devices->groupNames)
	{
		if (String::does_contain(_inputString.to_char(), dgn->name.to_char()))
		{
			_inputString = _inputString.replace(dgn->name.to_string(), String::empty()).trim();
			result |= dgn->id;
		}
	}

	if (result == 0)
	{
		result = Devices::all;
	}

	return result;
}

String Input::Devices::to_string(DeviceID _id, Optional<Hand::Type> _hand)
{
	an_assert(s_devices);

	String result;

	if (_id != Devices::all)
	{
		for_every(dgn, s_devices->groupNames)
		{
			if (is_whole_flag_set(_id, dgn->id))
			{
				result += dgn->name.to_string();
				result += String::space();
				clear_flag(_id, dgn->id);
			}
		}
		for_every(device, s_devices->devices)
		{
			if (is_flag_set(_id, device->id))
			{
				result += device->name.to_string();
				result += String::space();
				clear_flag(_id, device->id);
			}
		}

		result = result.trim();
	}

	if (_hand.is_set() && _hand.get() == Hand::Left)
	{
		result = String(TXT("left")) + String::space() + result;
	}

	if (_hand.is_set() && _hand.get() == Hand::Right)
	{
		result = String(TXT("right")) + String::space() + result;
	}

	return result;
}

Name const & Input::Devices::get_for_prompt_name(DeviceID _id)
{
	an_assert(s_devices);

	if (_id != Devices::all)
	{
		for_every(device, s_devices->devices)
		{
			if (is_flag_set(_id, device->id) &&
				device->forPromptName.is_valid())
			{
				return device->forPromptName;
			}
		}
		for_every(dgn, s_devices->groupNames)
		{
			if (is_flag_set(_id, dgn->id) &&
				dgn->forPromptName.is_valid())
			{
				return dgn->forPromptName;
			}
		}
	}

	return Name::invalid();
}

//

Input::Button::Type Input::Button::parse(String const& value)
{
	if (String::compare_icase(value, TXT("primary")) ||
		String::compare_icase(value, TXT("action")) ||
		String::compare_icase(value, TXT("a")) ||
		String::compare_icase(value, TXT("x"))) return Input::Button::Primary;
	if (String::compare_icase(value, TXT("secondary")) ||
		String::compare_icase(value, TXT("menu")) ||
		String::compare_icase(value, TXT("b")) ||
		String::compare_icase(value, TXT("y"))) return Input::Button::Secondary;
	if (String::compare_icase(value, TXT("systemMenu"))) return Input::Button::SystemMenu;
	if (String::compare_icase(value, TXT("grip"))) return Input::Button::Grip;
	if (String::compare_icase(value, TXT("pointerPinch"))) return Input::Button::PointerPinch;
	if (String::compare_icase(value, TXT("middlePinch"))) return Input::Button::MiddlePinch;
	if (String::compare_icase(value, TXT("ringPinch"))) return Input::Button::RingPinch;
	if (String::compare_icase(value, TXT("pinkyPinch"))) return Input::Button::PinkyPinch;
	if (String::compare_icase(value, TXT("trigger"))) return Input::Button::Trigger;
	if (String::compare_icase(value, TXT("tpad")) ||
		String::compare_icase(value, TXT("trackpad"))) return Input::Button::Trackpad;
	if (String::compare_icase(value, TXT("tpadTouch")) ||
		String::compare_icase(value, TXT("trackpadTouch"))) return Input::Button::TrackpadTouch;
	//
	if (String::compare_icase(value, TXT("inUse"))) return Input::Button::InUse;
	if (String::compare_icase(value, TXT("usesHandTracking"))) return Input::Button::UsesHandTracking;
	//
	if (String::compare_icase(value, TXT("pose"))) return Input::Button::Pose;
	if (String::compare_icase(value, TXT("upsideDown"))) return Input::Button::UpsideDown;
	if (String::compare_icase(value, TXT("insideToHead"))) return Input::Button::InsideToHead;
	//
	if (String::compare_icase(value, TXT("headSideTouch"))) return Input::Button::HeadSideTouch;
	//
	if (String::compare_icase(value, TXT("thumb"))) return Input::Button::Thumb;
	if (String::compare_icase(value, TXT("pointer"))) return Input::Button::Pointer;
	if (String::compare_icase(value, TXT("middle"))) return Input::Button::Middle;
	if (String::compare_icase(value, TXT("ring"))) return Input::Button::Ring;
	if (String::compare_icase(value, TXT("pinky"))) return Input::Button::Pinky;
	if (String::compare_icase(value, TXT("thumbFolded"))) return Input::Button::ThumbFolded;
	if (String::compare_icase(value, TXT("pointerFolded"))) return Input::Button::PointerFolded;
	if (String::compare_icase(value, TXT("middleFolded"))) return Input::Button::MiddleFolded;
	if (String::compare_icase(value, TXT("ringFolded"))) return Input::Button::RingFolded;
	if (String::compare_icase(value, TXT("pinkyFolded"))) return Input::Button::PinkyFolded;
	if (String::compare_icase(value, TXT("thumbStraight"))) return Input::Button::ThumbStraight;
	if (String::compare_icase(value, TXT("pointerStraight"))) return Input::Button::PointerStraight;
	if (String::compare_icase(value, TXT("middleStraight"))) return Input::Button::MiddleStraight;
	if (String::compare_icase(value, TXT("ringStraight"))) return Input::Button::RingStraight;
	if (String::compare_icase(value, TXT("pinkyStraight"))) return Input::Button::PinkyStraight;
	//
	if (String::compare_icase(value, TXT("d-padUp")) ||
		String::compare_icase(value, TXT("dPadUp"))) return Input::Button::DPadUp;
	if (String::compare_icase(value, TXT("d-padDown")) ||
		String::compare_icase(value, TXT("dPadDown"))) return Input::Button::DPadDown;
	if (String::compare_icase(value, TXT("d-padLeft")) ||
		String::compare_icase(value, TXT("dPadLeft"))) return Input::Button::DPadLeft;
	if (String::compare_icase(value, TXT("d-padRight")) ||
		String::compare_icase(value, TXT("dPadRight"))) return Input::Button::DPadRight;
	//
	if (String::compare_icase(value, TXT("joyUp")) ||
		String::compare_icase(value, TXT("joystickUp"))) return Input::Button::JoystickUp;
	if (String::compare_icase(value, TXT("joyDown")) ||
		String::compare_icase(value, TXT("joystickDown"))) return Input::Button::JoystickDown;
	if (String::compare_icase(value, TXT("joyLeft")) ||
		String::compare_icase(value, TXT("joystickLeft"))) return Input::Button::JoystickLeft;
	if (String::compare_icase(value, TXT("joyRight")) ||
		String::compare_icase(value, TXT("joystickRight"))) return Input::Button::JoystickRight;
	if (String::compare_icase(value, TXT("joyPress")) ||
		String::compare_icase(value, TXT("joystickPress"))) return Input::Button::JoystickPress;
	//
	if (String::compare_icase(value, TXT("t-padUp")) ||
		String::compare_icase(value, TXT("trackpadUp"))) return Input::Button::TrackpadUp;
	if (String::compare_icase(value, TXT("t-padDown")) ||
		String::compare_icase(value, TXT("trackpadDown"))) return Input::Button::TrackpadDown;
	if (String::compare_icase(value, TXT("t-padLeft")) ||
		String::compare_icase(value, TXT("trackpadLeft"))) return Input::Button::TrackpadLeft;
	if (String::compare_icase(value, TXT("t-padRight")) ||
		String::compare_icase(value, TXT("trackpadRight"))) return Input::Button::TrackpadRight;
	//
	if (String::compare_icase(value, TXT("t-padDirCentre")) ||
		String::compare_icase(value, TXT("trackpadDirCentre"))) return Input::Button::TrackpadDirCentre;
	if (String::compare_icase(value, TXT("t-padDirUp")) ||
		String::compare_icase(value, TXT("trackpadDirUp"))) return Input::Button::TrackpadDirUp;
	if (String::compare_icase(value, TXT("t-padDirDown")) ||
		String::compare_icase(value, TXT("trackpadDirDown"))) return Input::Button::TrackpadDirDown;
	if (String::compare_icase(value, TXT("t-padDirLeft")) ||
		String::compare_icase(value, TXT("trackpadDirLeft"))) return Input::Button::TrackpadDirLeft;
	if (String::compare_icase(value, TXT("t-padDirRight")) ||
		String::compare_icase(value, TXT("trackpadDirRight"))) return Input::Button::TrackpadDirRight;
	//
	if (value != String::empty())
	{
		error(TXT("button not found \"%S\""), value.to_char());
		todo_important(TXT("more Input::Buttons!"));
	}
	return Input::Button::None;
}

tchar const * Input::Button::to_char(Type type)
{
	if (type == Input::Button::Primary) return TXT("primary");
	if (type == Input::Button::Secondary) return TXT("secondary");
	if (type == Input::Button::SystemMenu) return TXT("systemMenu");
	if (type == Input::Button::Grip) return TXT("grip");
	if (type == Input::Button::PointerPinch) return TXT("pointerPinch");
	if (type == Input::Button::MiddlePinch) return TXT("middlePinch");
	if (type == Input::Button::RingPinch) return TXT("ringPinch");
	if (type == Input::Button::PinkyPinch) return TXT("pinkyPinch");
	if (type == Input::Button::Trigger) return TXT("trigger");
	if (type == Input::Button::Trackpad) return TXT("trackpad");
	if (type == Input::Button::TrackpadTouch) return TXT("trackpadTouch");
	//
	if (type == Input::Button::InUse) return TXT("inUse");
	if (type == Input::Button::UsesHandTracking) return TXT("usesHandTracking");
	//
	if (type == Input::Button::Pose) return TXT("pose");
	if (type == Input::Button::UpsideDown) return TXT("upsideDown");
	if (type == Input::Button::InsideToHead) return TXT("insideToHead");
	//
	if (type == Input::Button::HeadSideTouch) return TXT("headSideTouch");
	//
	if (type == Input::Button::Thumb) return TXT("thumb");
	if (type == Input::Button::Pointer) return TXT("pointer");
	if (type == Input::Button::Middle) return TXT("middle");
	if (type == Input::Button::Ring) return TXT("ring");
	if (type == Input::Button::Pinky) return TXT("pinky");
	if (type == Input::Button::ThumbFolded) return TXT("thumbFolded");
	if (type == Input::Button::PointerFolded) return TXT("pointerFolded");
	if (type == Input::Button::MiddleFolded) return TXT("middleFolded");
	if (type == Input::Button::RingFolded) return TXT("ringFolded");
	if (type == Input::Button::PinkyFolded) return TXT("pinkyFolded");
	if (type == Input::Button::ThumbStraight) return TXT("thumbStraight");
	if (type == Input::Button::PointerStraight) return TXT("pointerStraight");
	if (type == Input::Button::MiddleStraight) return TXT("middleStraight");
	if (type == Input::Button::RingStraight) return TXT("ringStraight");
	if (type == Input::Button::PinkyStraight) return TXT("pinkyStraight");
	//
	if (type == Input::Button::DPadUp) return TXT("dPadUp");
	if (type == Input::Button::DPadDown) return TXT("dPadDown");
	if (type == Input::Button::DPadLeft) return TXT("dPadLeft");
	if (type == Input::Button::DPadRight) return TXT("dPadRight");
	//
	if (type == Input::Button::JoystickUp) return TXT("joystickUp");
	if (type == Input::Button::JoystickDown) return TXT("joystickDown");
	if (type == Input::Button::JoystickLeft) return TXT("joystickLeft");
	if (type == Input::Button::JoystickRight) return TXT("joystickRight");
	if (type == Input::Button::JoystickPress) return TXT("joystickPress");
	//
	if (type == Input::Button::TrackpadUp) return TXT("trackpadUp");
	if (type == Input::Button::TrackpadDown) return TXT("trackpadDown");
	if (type == Input::Button::TrackpadLeft) return TXT("trackpadLeft");
	if (type == Input::Button::TrackpadRight) return TXT("trackpadRight");
	//
	if (type == Input::Button::TrackpadDirCentre) return TXT("trackpadDirCentre");
	if (type == Input::Button::TrackpadDirUp) return TXT("trackpadDirUp");
	if (type == Input::Button::TrackpadDirDown) return TXT("trackpadDirDown");
	if (type == Input::Button::TrackpadDirLeft) return TXT("trackpadDirLeft");
	if (type == Input::Button::TrackpadDirRight) return TXT("trackpadDirRight");
	//
	return TXT("");
}

Input::Button::WithSource Input::Button::WithSource::parse(String const & _value, Input::Button::Type _default)
{
	String value = _value;
	DeviceID source = Input::Devices::parse(REF_ value);
	Optional<Hand::Type> hand;

	value = value.trim();

	bool released = false;

	if (String::does_contain(value.to_char(), TXT("left")))
	{
		hand = Hand::Left;
		value = value.replace(String(TXT("left")), String::empty());
		value = value.trim();
	}
	else if (String::does_contain(value.to_char(), TXT("right")))
	{
		hand = Hand::Right;
		value = value.replace(String(TXT("right")), String::empty());
		value = value.trim();
	}

	if (String::does_contain(value.to_char(), TXT("released")))
	{
		released = true;
		value = value.replace(String(TXT("released")), String::empty());
		value = value.trim();
	}

	Button::Type button = Button::parse(value);
	if (button != Button::None)
	{
		return WithSource(button, source, hand, released);
	}
	//
	return WithSource(_default, source, hand, false);
}

String Input::Button::WithSource::to_string(WithSource _button)
{
	return Input::Devices::to_string(_button.source, _button.hand) + String::space() + to_char(_button.type) + String::space() + _button.released_to_string();
}

Input::Button::WithSource Input::Button::WithSource::from_stick(Input::Stick::WithSource _stick, Input::StickDir::Type _dir)
{
	if (_stick.type == Input::Stick::DPad)
	{
		if (_dir == Input::StickDir::Left) return WithSource(Input::Button::DPadLeft, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Right) return WithSource(Input::Button::DPadRight, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Down) return WithSource(Input::Button::DPadDown, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Up) return WithSource(Input::Button::DPadUp, _stick.source, _stick.hand, false);
	}
	if (_stick.type == Input::Stick::Joystick)
	{
		if (_dir == Input::StickDir::Left) return WithSource(Input::Button::JoystickLeft, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Right) return WithSource(Input::Button::JoystickRight, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Down) return WithSource(Input::Button::JoystickDown, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Up) return WithSource(Input::Button::JoystickUp, _stick.source, _stick.hand, false);
	}
	if (_stick.type == Input::Stick::Trackpad)
	{
		if (_dir == Input::StickDir::Left) return WithSource(Input::Button::TrackpadLeft, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Right) return WithSource(Input::Button::TrackpadRight, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Down) return WithSource(Input::Button::TrackpadDown, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Up) return WithSource(Input::Button::TrackpadUp, _stick.source, _stick.hand, false);
	}
	if (_stick.type == Input::Stick::TrackpadDir)
	{
		if (_dir == Input::StickDir::Left) return WithSource(Input::Button::TrackpadDirLeft, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Right) return WithSource(Input::Button::TrackpadDirRight, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Down) return WithSource(Input::Button::TrackpadDirDown, _stick.source, _stick.hand, false);
		if (_dir == Input::StickDir::Up) return WithSource(Input::Button::TrackpadDirUp, _stick.source, _stick.hand, false);
	}
	return WithSource(Input::Button::None, _stick.source, _stick.hand, false);
}

//

tchar const* Input::Stick::to_char(Type _type)
{
	if (_type == Input::Stick::DPad) return TXT("d-pad");
	if (_type == Input::Stick::Joystick) return TXT("joystick");
	if (_type == Input::Stick::Trackpad) return TXT("trackpad");
	if (_type == Input::Stick::TrackpadTouch) return TXT("trackpadTouch");
	if (_type == Input::Stick::TrackpadDir) return TXT("trackpadDir");
	return TXT("");
}

Input::Stick::WithSource Input::Stick::WithSource::parse(String const & _value, Input::Stick::Type _default)
{
	String value = _value;
	DeviceID source = Input::Devices::parse(REF_ value);
	Optional<Hand::Type> hand;

	value = value.trim();

	if (String::does_contain(value.to_char(), TXT("left")))
	{
		hand = Hand::Left;
		value = value.replace(String(TXT("left")), String::empty());
		value = value.trim();
	}
	else if (String::does_contain(value.to_char(), TXT("right")))
	{
		hand = Hand::Right;
		value = value.replace(String(TXT("right")), String::empty());
		value = value.trim();
	}

	if (String::compare_icase(value, TXT("dpad")) ||
		String::compare_icase(value, TXT("d-pad"))) return WithSource(Input::Stick::DPad, source, hand);
	if (String::compare_icase(value, TXT("stick")) ||
		String::compare_icase(value, TXT("joystick"))) return WithSource(Input::Stick::Joystick, source, hand);
	if (String::compare_icase(value, TXT("tpad")) ||
		String::compare_icase(value, TXT("trackpad")) ||
		String::compare_icase(value, TXT("trackpad"))) return WithSource(Input::Stick::Trackpad, source, hand);
	if (String::compare_icase(value, TXT("tpadTouch")) ||
		String::compare_icase(value, TXT("tpadtouch")) ||
		String::compare_icase(value, TXT("trackpadTouch")) ||
		String::compare_icase(value, TXT("trackpadtouch"))) return WithSource(Input::Stick::TrackpadTouch, source, hand);
	if (String::compare_icase(value, TXT("tpaddir")) ||
		String::compare_icase(value, TXT("trackpadDir")) ||
		String::compare_icase(value, TXT("trackpaddir"))) return WithSource(Input::Stick::TrackpadDir, source, hand);
	//
	return WithSource(_default, source);
}

String Input::Stick::WithSource::to_string(Input::Stick::WithSource _stick)
{
	return Input::Devices::to_string(_stick.source, _stick.hand) + String::space() + to_char(_stick.type);
}

//

Input::StickDir::Type Input::StickDir::parse(String const & _value, Input::StickDir::Type _default)
{
	if (String::compare_icase(_value, TXT("left"))) return Input::StickDir::Left;
	if (String::compare_icase(_value, TXT("right"))) return Input::StickDir::Right;
	if (String::compare_icase(_value, TXT("down"))) return Input::StickDir::Down;
	if (String::compare_icase(_value, TXT("up"))) return Input::StickDir::Up;
	//
	return _default;
}

String Input::StickDir::to_string(Input::StickDir::Type _stick)
{
	if (_stick == Input::StickDir::Left) return String(TXT("left"));
	if (_stick == Input::StickDir::Right) return String(TXT("right"));
	if (_stick == Input::StickDir::Down) return String(TXT("down"));
	if (_stick == Input::StickDir::Up) return String(TXT("up"));
	//
	return String::empty();
}

//

Input::Mouse::WithSource Input::Mouse::WithSource::parse(String const & _value)
{
	String value = _value;
	DeviceID source = Input::Devices::parse(REF_ value);
	Optional<Hand::Type> hand;

	value = value.trim();

	if (String::does_contain(value.to_char(), TXT("left")))
	{
		hand = Hand::Left;
	}
	else if (String::does_contain(value.to_char(), TXT("right")))
	{
		hand = Hand::Right;
	}

	return WithSource(source, hand);
}

String Input::Mouse::WithSource::to_string(Input::Mouse::WithSource _mouse)
{
	return Input::Devices::to_string(_mouse.source, _mouse.hand);
}
