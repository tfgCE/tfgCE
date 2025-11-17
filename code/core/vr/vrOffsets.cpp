#include "vrOffsets.h"

#include "iVR.h"

#include "..\math\math.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace VR;

//

struct VRSystem
{
	Name name;
	Name sameAs;
	Name inputDeviceName;
	CACHED_ Optional<VR::Input::DeviceID> inputDeviceID; // filled only for currentVRSystem
	Transform leftHand = Transform::identity;
	Transform rightHand = Transform::identity;
	bool handTrackingProvided = false; // only to know if we should save it or not
	Transform leftHandTracking = Transform::identity;
	Transform rightHandTracking = Transform::identity;
	Transform leftFingerTracking = Transform::identity;
	Transform rightFingerTracking = Transform::identity;
};

#ifdef BUILD_NON_RELEASE
bool keepCurrentVRSystem = false;
#endif

VRSystem currentVRSystem[Hand::MAX]; // two different systems per hand
VRSystem vrSystems[32];
int vrSystemCount = 0;

Transform customVROffsets[Hand::MAX];

#ifdef BUILD_NON_RELEASE
Transform& Offsets::dev_access_hand(Hand::Type _hand)
{
	keepCurrentVRSystem = true;

	auto& cvrs = currentVRSystem[_hand];
	return _hand == Hand::Left ? cvrs.leftHand : cvrs.rightHand;
}
#endif

void update_if_required(Hand::Type _hand)
{
#ifdef BUILD_NON_RELEASE
	if (keepCurrentVRSystem)
	{
		// do not change anything
		return;
	}
#endif
	Name vrSystemName = IVR::get()->get_name();
	VR::Input::DeviceID controllerSourceInputDeviceID = IVR::get()->get_controls().hands[_hand].source;
	if (vrSystemName == currentVRSystem[_hand].name &&
		currentVRSystem[_hand].inputDeviceID.is_set() &&
		currentVRSystem[_hand].inputDeviceID.get() == controllerSourceInputDeviceID)
	{
		return;
	}
	Offsets::apply_custom_offsets();
	for_count(int, ignoreDeviceID, 2)
	{
		Name controllerSourceInputDeviceName;
		if (! ignoreDeviceID)
		{
			if (auto* dev = VR::Input::Devices::get(controllerSourceInputDeviceID))
			{
				controllerSourceInputDeviceName = dev->name;
			}
		}
		VRSystem const* vrSystem = vrSystems;
		for_count(int, i, vrSystemCount)
		{
			if (vrSystem->name == vrSystemName)
			{
				bool ok = true;
				if (!ignoreDeviceID)
				{
					ok = controllerSourceInputDeviceName.is_valid() && vrSystem->inputDeviceName == controllerSourceInputDeviceName;
				}
				if (ok)
				{
					currentVRSystem[_hand] = *vrSystem;
					currentVRSystem[_hand].inputDeviceID = controllerSourceInputDeviceID; // cache it!
					return;
				}
			}
			++vrSystem;
		}
	}
	error(TXT("vr system \"%S\" not recognised"), vrSystemName.to_char());
}

void Offsets::apply_custom_offsets()
{
	customVROffsets[Hand::Left] = MainConfig::global().get_vr_offset(Hand::Left).to_transform();
	customVROffsets[Hand::Right] = MainConfig::global().get_vr_offset(Hand::Right).to_transform();
}

Transform Offsets::get_left_hand()
{
	update_if_required(Hand::Left);
	return customVROffsets[Hand::Left].to_world(currentVRSystem[Hand::Left].leftHand);
}

Transform Offsets::get_right_hand()
{
	update_if_required(Hand::Right);
	return customVROffsets[Hand::Right].to_world(currentVRSystem[Hand::Right].rightHand);
}

Transform Offsets::get_hand(Hand::Type _hand)
{
	return _hand == Hand::Left ? get_left_hand() : get_right_hand();
}

Transform Offsets::get_left_hand_tracking()
{
	update_if_required(Hand::Left);
	return currentVRSystem[Hand::Left].leftHandTracking;
}

Transform Offsets::get_right_hand_tracking()
{
	update_if_required(Hand::Right);
	return currentVRSystem[Hand::Right].rightHandTracking;
}

Transform Offsets::get_hand_tracking(Hand::Type _hand)
{
	return _hand == Hand::Left ? get_left_hand_tracking() : get_right_hand_tracking();
}

Transform Offsets::get_left_finger_tracking()
{
	update_if_required(Hand::Left);
	return currentVRSystem[Hand::Left].leftFingerTracking;
}

Transform Offsets::get_right_finger_tracking()
{
	update_if_required(Hand::Right);
	return currentVRSystem[Hand::Right].rightFingerTracking;
}

Transform Offsets::get_finger_tracking(Hand::Type _hand)
{
	return _hand == Hand::Left ? get_left_finger_tracking() : get_right_finger_tracking();
}

bool Offsets::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("vrSystem")))
	{
		Name vrSystemName = node->get_name_attribute(TXT("name"));
		Name inputDeviceName = node->get_name_attribute(TXT("inputDevice"));
		if (vrSystemName.is_valid())
		{
			int vrSystemIdx = vrSystemCount;
			for (int i = 0; i < vrSystemCount; ++i)
			{
				if (vrSystems[i].name == vrSystemName &&
					vrSystems[i].inputDeviceName == inputDeviceName)
				{
					vrSystemIdx = i;
					break;
				}
			}
			auto & vrSystem = vrSystems[vrSystemIdx];
			vrSystem.name = vrSystemName;
			vrSystem.inputDeviceName = inputDeviceName;
			vrSystem.sameAs = node->get_name_attribute(TXT("sameAs"));
			if (vrSystem.sameAs.is_valid())
			{
				bool found = false;
				for_count(int, i, vrSystemCount)
				{
					if (vrSystems[i].name == vrSystem.sameAs)
					{
						found = true;
						vrSystem.leftHand = vrSystems[i].leftHand;
						vrSystem.rightHand = vrSystems[i].rightHand;
						vrSystem.handTrackingProvided = vrSystems[i].handTrackingProvided;
						vrSystem.leftHandTracking = vrSystems[i].leftHandTracking;
						vrSystem.rightHandTracking = vrSystems[i].rightHandTracking;
						vrSystem.leftFingerTracking = vrSystems[i].leftFingerTracking;
						vrSystem.rightFingerTracking = vrSystems[i].rightFingerTracking;
					}
				}
				error_loading_xml_on_assert(found, result, node, TXT("couldn't find sameAs=\"%S\" vr system"), vrSystem.sameAs.to_char());
			}
			// not else, always load
			{
				for_every(cNode, node->children_named(TXT("forControllers")))
				{
					for_every(hNode, cNode->children_named(TXT("handLeft")))
					{
						vrSystem.leftHand = Transform::identity;
						result &= vrSystem.leftHand.load_from_xml(hNode);
					}

					for_every(hNode, cNode->children_named(TXT("handRight")))
					{
						vrSystem.rightHand = Transform::identity;
						result &= vrSystem.rightHand.load_from_xml(hNode);
					}
				}

				for_every(cNode, node->children_named(TXT("forHandTracking")))
				{
					vrSystem.handTrackingProvided = true;

					for_every(hNode, cNode->children_named(TXT("handLeft")))
					{
						vrSystem.leftHandTracking = Transform::identity;
						result &= vrSystem.leftHandTracking.load_from_xml(hNode);
					}

					for_every(hNode, cNode->children_named(TXT("handRight")))
					{
						vrSystem.rightHandTracking = Transform::identity;
						result &= vrSystem.rightHandTracking.load_from_xml(hNode);
					}

					for_every(hNode, cNode->children_named(TXT("fingerLeft")))
					{
						vrSystem.leftFingerTracking = Transform::identity;
						result &= vrSystem.leftFingerTracking.load_from_xml(hNode);
					}

					for_every(hNode, cNode->children_named(TXT("fingerRight")))
					{
						vrSystem.rightFingerTracking = Transform::identity;
						result &= vrSystem.rightFingerTracking.load_from_xml(hNode);
					}
				}
			}

			vrSystemCount = max(vrSystemIdx + 1, vrSystemCount);
		}
	}

	return result;
}

bool Offsets::save_to_xml(IO::XML::Node * _node)
{
	bool result = true;

	if (IO::XML::Node* node = _node->add_node(TXT("vrOffsets")))
	{
		VRSystem const * vrSystem = vrSystems;
		for_count(int, i, vrSystemCount)
		{
			if (IO::XML::Node* vrsNode = node->add_node(TXT("vrSystem")))
			{
				vrsNode->set_attribute(TXT("name"), vrSystem->name.to_string());
				if (vrSystem->inputDeviceName.is_valid())
				{
					vrsNode->set_attribute(TXT("inputDevice"), vrSystem->inputDeviceName.to_string());
				}
				if (vrSystem->sameAs.is_valid())
				{
					vrsNode->set_attribute(TXT("sameAs"), vrSystem->sameAs.to_string());
				}
				else
				{
					if (auto* cNode = vrsNode->add_node(TXT("forControllers")))
					{
						vrSystem->leftHand.save_to_xml_child_node(cNode, TXT("handLeft"));
						vrSystem->rightHand.save_to_xml_child_node(cNode, TXT("handRight"));
					}
					if (vrSystem->handTrackingProvided)
					{
						if (auto* cNode = vrsNode->add_node(TXT("forHandTracking")))
						{
							vrSystem->leftHandTracking.save_to_xml_child_node(cNode, TXT("handLeft"));
							vrSystem->rightHandTracking.save_to_xml_child_node(cNode, TXT("handRight"));
							vrSystem->leftFingerTracking.save_to_xml_child_node(cNode, TXT("fingerLeft"));
							vrSystem->rightFingerTracking.save_to_xml_child_node(cNode, TXT("fingerRight"));
						}
					}
				}
			}
			++vrSystem;
		}
	}

	return result;
}
