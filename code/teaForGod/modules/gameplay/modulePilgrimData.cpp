#include "modulePilgrimData.h"

#include "..\..\..\framework\library\usedLibraryStored.inl"

#include "..\..\library\library.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(ModulePilgrimData);

ModulePilgrimData::ModulePilgrimData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ModulePilgrimData::~ModulePilgrimData()
{
}

bool ModulePilgrimData::read_parameter_from(IO::XML::Attribute const* _attr, Framework::LibraryLoadingContext& _lc)
{
	if (_attr->get_name() == TXT("pilgrimKeeperActorType"))
	{
		return pilgrimKeeperActorType.load_from_xml(_attr, _lc);
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool ModulePilgrimData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("leftHand"))
	{
		bool result = true;
		result &= leftHand.actorType.load_from_xml(_node, TXT("actorType"), _lc);
		leftHand.attachToSocket = _node->get_name_attribute(TXT("attachToSocket"), leftHand.attachToSocket);
		leftForearm.forearmStartSocket = _node->get_name_attribute(TXT("forearmStartSocket"), leftForearm.forearmStartSocket);
		leftForearm.forearmEndSocket = _node->get_name_attribute(TXT("forearmEndSocket"), leftForearm.forearmEndSocket);
		leftForearm.handOverSocket = _node->get_name_attribute(TXT("handOverForearmSocket"), leftForearm.handOverSocket);
		leftHandForearmDisplay.actorType.load_from_xml(_node, TXT("forearmDisplayActorType"), _lc);
		leftHandForearmDisplay.attachToSocket = _node->get_name_attribute(TXT("forearmDisplayAttachToSocket"), leftHandForearmDisplay.attachToSocket);
		leftHandForearmDisplayAttachToHand = _node->get_bool_attribute_or_from_child_presence(TXT("forearmDisplayAttachToHand"), leftHandForearmDisplayAttachToHand);
		return result;
	}
	else if (_node->get_name() == TXT("rightHand"))
	{
		bool result = true;
		result &= rightHand.actorType.load_from_xml(_node, TXT("actorType"), _lc);
		rightHand.attachToSocket = _node->get_name_attribute(TXT("attachToSocket"), rightHand.attachToSocket);
		rightForearm.forearmStartSocket = _node->get_name_attribute(TXT("forearmStartSocket"), rightForearm.forearmStartSocket);
		rightForearm.forearmEndSocket = _node->get_name_attribute(TXT("forearmEndSocket"), rightForearm.forearmEndSocket);
		rightForearm.handOverSocket = _node->get_name_attribute(TXT("handOverForearmSocket"), rightForearm.handOverSocket);
		rightHandForearmDisplay.actorType.load_from_xml(_node, TXT("forearmDisplayActorType"), _lc);
		rightHandForearmDisplay.attachToSocket = _node->get_name_attribute(TXT("forearmDisplayAttachToSocket"), rightHandForearmDisplay.attachToSocket);
		rightHandForearmDisplayAttachToHand = _node->get_bool_attribute_or_from_child_presence(TXT("forearmDisplayAttachToHand"), rightHandForearmDisplayAttachToHand);
		return result;
	}
	else if (_node->get_name() == TXT("handHoldPoint"))
	{
		bool result = true;
		handHoldPointSockets.any = _node->get_name_attribute(TXT("socket"), handHoldPointSockets.any);
		if (!handHoldPointSockets.any.is_valid())
		{
			error_loading_xml(_node, TXT("no socket for hand hold point"));
			result = false;
		}
		return result;
	}
	else if (_node->get_name() == TXT("leftHandHoldPoint"))
	{
		bool result = true;
		handHoldPointSockets.left = _node->get_name_attribute(TXT("socket"), handHoldPointSockets.left);
		return result;
	}
	else if (_node->get_name() == TXT("rightHandHoldPoint"))
	{
		bool result = true;
		handHoldPointSockets.right= _node->get_name_attribute(TXT("socket"), handHoldPointSockets.right);
		return result;
	}
	else if (_node->get_name() == TXT("handGrabPoint"))
	{
		bool result = true;
		handGrabPointSockets.any = _node->get_name_attribute(TXT("socket"), handGrabPointSockets.any);
		return result;
	}
	else if (_node->get_name() == TXT("leftHandGrabPoint"))
	{
		bool result = true;
		handGrabPointSockets.left = _node->get_name_attribute(TXT("socket"), handGrabPointSockets.left);
		leftHandGrabPoint.axis = Axis::parse_from(_node->get_string_attribute(TXT("grabAxis")), leftHandGrabPoint.axis);
		leftHandGrabPoint.dirAxis = Axis::parse_from(_node->get_string_attribute(TXT("grabDirAxis")), leftHandGrabPoint.dirAxis);
		leftHandGrabPoint.dirSign = _node->get_float_attribute(TXT("grabDirSign"), leftHandGrabPoint.dirSign);
		return result;
	}
	else if (_node->get_name() == TXT("rightHandGrabPoint"))
	{
		bool result = true;
		handGrabPointSockets.right = _node->get_name_attribute(TXT("socket"), handGrabPointSockets.right);
	
		rightHandGrabPoint.axis = Axis::parse_from(_node->get_string_attribute(TXT("grabAxis")), rightHandGrabPoint.axis);
		rightHandGrabPoint.dirAxis = Axis::parse_from(_node->get_string_attribute(TXT("grabDirAxis")), rightHandGrabPoint.dirAxis);
		rightHandGrabPoint.dirSign = _node->get_float_attribute(TXT("grabDirSign"), rightHandGrabPoint.dirSign);
		return result;
	}
	else if (_node->get_name() == TXT("handGrabPointDial"))
	{
		bool result = true;
		handGrabPointDialSockets.any = _node->get_name_attribute(TXT("socket"), handGrabPointDialSockets.any);
		return result;
	}
	else if (_node->get_name() == TXT("leftHandGrabPointDial"))
	{
		bool result = true;
		handGrabPointDialSockets.left = _node->get_name_attribute(TXT("socket"), handGrabPointDialSockets.left);
		leftHandGrabPointDial.axis = Axis::parse_from(_node->get_string_attribute(TXT("grabAxis")), leftHandGrabPointDial.axis);
		leftHandGrabPointDial.axisSign = _node->get_float_attribute(TXT("grabAxisSign"), leftHandGrabPointDial.axisSign);
		leftHandGrabPointDial.dirAxis = Axis::parse_from(_node->get_string_attribute(TXT("grabDirAxis")), leftHandGrabPointDial.dirAxis);
		leftHandGrabPointDial.dirSign = _node->get_float_attribute(TXT("grabDirSign"), leftHandGrabPointDial.dirSign);
		return result;
	}
	else if (_node->get_name() == TXT("rightHandGrabPointDial"))
	{
		bool result = true;
		handGrabPointDialSockets.right = _node->get_name_attribute(TXT("socket"), handGrabPointDialSockets.right);
		rightHandGrabPointDial.axis = Axis::parse_from(_node->get_string_attribute(TXT("grabAxis")), rightHandGrabPointDial.axis);
		rightHandGrabPointDial.axisSign = _node->get_float_attribute(TXT("grabAxisSign"), rightHandGrabPointDial.axisSign);
		rightHandGrabPointDial.dirAxis = Axis::parse_from(_node->get_string_attribute(TXT("grabDirAxis")), rightHandGrabPointDial.dirAxis);
		rightHandGrabPointDial.dirSign = _node->get_float_attribute(TXT("grabDirSign"), rightHandGrabPointDial.dirSign);
		return result;
	}
	else if (_node->get_name() == TXT("portHoldPoint"))
	{
		bool result = true;
		portHoldPointSockets.any = _node->get_name_attribute(TXT("socket"), portHoldPointSockets.any);
		if (!portHoldPointSockets.any.is_valid())
		{
			error_loading_xml(_node, TXT("no socket for port hold point"));
			result = false;
		}
		return result;
	}
	else if (_node->get_name() == TXT("leftPortHoldPoint"))
	{
		bool result = true;
		portHoldPointSockets.left = _node->get_name_attribute(TXT("socket"), portHoldPointSockets.left);
		return result;
	}
	else if (_node->get_name() == TXT("rightPortHoldPoint"))
	{
		bool result = true;
		portHoldPointSockets.right = _node->get_name_attribute(TXT("socket"), portHoldPointSockets.right);
		return result;
	}
	else if (_node->get_name() == TXT("leftRestPoint"))
	{
		bool result = true;
		result &= leftRestPoint.actorType.load_from_xml(_node, TXT("actorType"), _lc);
		leftRestPoint.attachToSocket = _node->get_name_attribute(TXT("attachToSocket"), leftRestPoint.attachToSocket);
		return result;
	}
	else if (_node->get_name() == TXT("rightRestPoint"))
	{
		bool result = true;
		result &= rightRestPoint.actorType.load_from_xml(_node, TXT("actorType"), _lc);
		rightRestPoint.attachToSocket = _node->get_name_attribute(TXT("attachToSocket"), rightRestPoint.attachToSocket);
		return result;
	}
	else if (_node->get_name() == TXT("restHoldPoint"))
	{
		bool result = true;
		restHoldPointSockets.any = _node->get_name_attribute(TXT("socket"), restHoldPointSockets.any);
		if (!restHoldPointSockets.any.is_valid())
		{
			error_loading_xml(_node, TXT("no socket for rest hold point"));
			result = false;
		}
		return result;
	}
	else if (_node->get_name() == TXT("leftRestHoldPoint"))
	{
		bool result = true;
		restHoldPointSockets.left = _node->get_name_attribute(TXT("socket"), restHoldPointSockets.left);
		return result;
	}
	else if (_node->get_name() == TXT("rightRestHoldPoint"))
	{
		bool result = true;
		restHoldPointSockets.right = _node->get_name_attribute(TXT("socket"), restHoldPointSockets.right);
		return result;
	}
	else if (_node->get_name() == TXT("energyInhale"))
	{
		bool result = true;
		energyInhaleSocket = _node->get_name_attribute(TXT("socket"), energyInhaleSocket);
		return result;
	}
	else if (_node->get_name() == TXT("headMenu"))
	{
		bool result = true;
		headMenuSocket = _node->get_name_attribute(TXT("socket"), headMenuSocket);
		headMenuHandTriggerSocket = _node->get_name_attribute(TXT("handTriggerSocket"), headMenuHandTriggerSocket);
		return result;
	}
	else if (_node->get_name() == TXT("physicalViolence"))
	{
		bool result = true;
		physicalViolenceDirSocket = _node->get_name_attribute(TXT("dirSocket"), physicalViolenceDirSocket);
		result &= physicalViolencePulse.load_from_xml(_node, TXT("vrPulse"));
		result &= strongPhysicalViolencePulse.load_from_xml(_node, TXT("strongVRPulse"));
		return result;
	}
	else if (_node->get_name() == TXT("pockets"))
	{
		bool result = true;
		pocketsMaterialIndex.load_from_xml(_node, TXT("materialIndex"));
		pocketsMaterialParam = _node->get_name_attribute(TXT("materialParam"), pocketsMaterialParam);
		return result;
	}
	else if (_node->get_name() == TXT("pocket"))
	{
		bool result = true;
		if (pockets.get_size() < MAX_POCKETS)
		{
			Pocket pocket;
			pocket.name = _node->get_name_attribute(TXT("name"), pocket.name);
			pocket.pocketLevel = _node->get_int_attribute(TXT("pocketLevel"), pocket.pocketLevel);
			pocket.socket = _node->get_name_attribute(TXT("socket"), pocket.socket);
			if (! _node->get_string_attribute(TXT("side")).is_empty())
			{
				pocket.side = Hand::parse(_node->get_string_attribute(TXT("side")));
			}
			pocket.materialParamIndex = _node->get_int_attribute(TXT("materialParamIndex"), pocket.materialParamIndex);
			error_loading_xml_on_assert(pocket.name.is_valid(), result, _node, TXT("name required for pocket"));
			error_loading_xml_on_assert(pocket.socket.is_valid(), result, _node, TXT("socket required for pocket"));
			if (auto* attr = _node->get_attribute(TXT("objectHoldSocket")))
			{
				pocket.objectHoldSockets.push_back(attr->get_as_name());
			}
			for_every(node, _node->children_named(TXT("objectHold")))
			{
				Name ohs = node->get_name_attribute(TXT("socket"));
				if (ohs.is_valid())
				{
					pocket.objectHoldSockets.push_back(ohs);
				}
			}
			pockets.push_back(pocket);
		}
		else
		{
			error_loading_xml(_node, TXT("increase MAX_POCKETS"));
			result = false;
		}
		return result;
	}
	else if (_node->get_name() == TXT("equipmentParameters"))
	{
		bool result = true;
		result &= equipmentParameters.load_from_xml(_node);
		_lc.load_group_into(equipmentParameters);
		return result;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool ModulePilgrimData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= pilgrimKeeperActorType.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	result &= leftHand.actorType.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= rightHand.actorType.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
		
	result &= leftHandForearmDisplay.actorType.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= rightHandForearmDisplay.actorType.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
		
	result &= leftRestPoint.actorType.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= rightRestPoint.actorType.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	return result;
}
