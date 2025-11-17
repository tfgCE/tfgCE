#include "pilgrimageDevice.h"

#include "pilgrimageEnvironment.h"

#include "..\library\library.h"

#include "..\..\framework\library\usedLibraryStored.inl"

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(PilgrimageDevice);
LIBRARY_STORED_DEFINE_TYPE(PilgrimageDevice, pilgrimageDevice);

PilgrimageDevice::PilgrimageDevice(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
	for_count(int, i, EnergyType::MAX)
	{
		providesEnergy[i] = false;
	}
}

PilgrimageDevice::~PilgrimageDevice()
{
}

bool PilgrimageDevice::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= sceneryType.load_from_xml(_node, TXT("sceneryType"), _lc);
	result &= roomType.load_from_xml(_node, TXT("roomType"), _lc);

	openWorldCellSlot = _node->get_name_attribute_or_from_child(TXT("openWorldCellSlot"), openWorldCellSlot);
	openWorldCellSlotSecondary = _node->get_name_attribute_or_from_child(TXT("openWorldCellSlotSecondary"), openWorldCellSlotSecondary);
	result &= openWorldCellTagged.load_from_xml_attribute_or_child_node(_node, TXT("openWorldCellTagged"));
	openWorldCellDistTag = _node->get_name_attribute_or_from_child(TXT("openWorldCellDistTag"), openWorldCellDistTag);

	openWorldDistance.load_from_xml(_node, TXT("openWorldDistance"));

	directionTexturePart.load_from_xml(_node, TXT("directionTexturePart"), _lc);

	neverAppearAsDepletedForAutoMapOnly = _node->get_bool_attribute_or_from_child_presence(TXT("neverAppearAsDepletedForAutoMapOnly"), neverAppearAsDepletedForAutoMapOnly);
	neverAppearAsDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("neverAppearAsDepleted"), neverAppearAsDepleted);
	reverseAppearAsDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("reverseAppearAsDepleted"), reverseAppearAsDepleted);
	appearOnMap = ! _node->get_bool_attribute_or_from_child_presence(TXT("doNotAppearOnMap"), ! appearOnMap);
	appearOnMap = _node->get_bool_attribute_or_from_child_presence(TXT("appearOnMap"), appearOnMap);
	appearOnMapOnlyIfNotDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("appearOnMapOnlyIfNotDepleted"), appearOnMapOnlyIfNotDepleted);
	appearOnMapOnlyIfDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("appearOnMapOnlyIfDepleted"), appearOnMapOnlyIfDepleted);
	appearOnMapOnlyIfVisited = _node->get_bool_attribute_or_from_child_presence(TXT("appearOnMapOnlyIfVisited"), appearOnMapOnlyIfVisited);
	
	knownFromStart = _node->get_bool_attribute_or_from_child_presence(TXT("knownFromStart"), knownFromStart);
	unobfuscatedFromStart = _node->get_bool_attribute_or_from_child_presence(TXT("unobfuscatedFromStart"), unobfuscatedFromStart);

	priority = _node->get_int_attribute_or_from_child(TXT("priority"), priority);
	chance = _node->get_float_attribute_or_from_child(TXT("chance"), chance);

	special = _node->get_bool_attribute_or_from_child_presence(TXT("special"), special);
	
	system = _node->get_name_attribute_or_from_child(TXT("system"), system);
	
	revealedUponEntrance = _node->get_bool_attribute_or_from_child_presence(TXT("revealedUponEntrance"), revealedUponEntrance);

	guidanceBeacon = _node->get_bool_attribute_or_from_child_presence(TXT("guidanceBeacon"), guidanceBeacon);
	guidanceBeaconIfNotDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("guidanceBeaconIfNotDepleted"), guidanceBeaconIfNotDepleted);
	if (guidanceBeaconIfNotDepleted)
	{
		guidanceBeacon = true;
	}

	longDistanceDetectable = _node->get_bool_attribute_or_from_child_presence(TXT("longDistanceDetectable"), longDistanceDetectable);
	longDistanceDetectableOnlyIfNotDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("longDistanceDetectableOnlyIfNotDepleted"), longDistanceDetectableOnlyIfNotDepleted);
	longDistanceDetectableByDirection = _node->get_bool_attribute_or_from_child_presence(TXT("longDistanceDetectableByDirection"), longDistanceDetectableByDirection);
	if (longDistanceDetectableOnlyIfNotDepleted || longDistanceDetectableByDirection)
	{
		longDistanceDetectable = true;
	}

	showDoorDirections = _node->get_bool_attribute_or_from_child_presence(TXT("showDoorDirections"), showDoorDirections);
	showDoorDirections = ! _node->get_bool_attribute_or_from_child_presence(TXT("noDoorDirections"), ! showDoorDirections);
	showDoorDirectionsOnlyIfDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("showDoorDirectionsOnlyIfDepleted"), showDoorDirectionsOnlyIfDepleted);
	if (showDoorDirectionsOnlyIfDepleted)
	{
		showDoorDirections = true;
	}
	showDoorDirectionsOnlyIfNotDepleted = _node->get_bool_attribute_or_from_child_presence(TXT("showDoorDirectionsOnlyIfNotDepleted"), showDoorDirectionsOnlyIfNotDepleted);
	if (showDoorDirectionsOnlyIfNotDepleted)
	{
		showDoorDirections = true;
	}
	showDoorDirectionsOnlyIfVisited = _node->get_bool_attribute_or_from_child_presence(TXT("showDoorDirectionsOnlyIfVisited"), showDoorDirectionsOnlyIfVisited);
	if (showDoorDirectionsOnlyIfVisited)
	{
		showDoorDirections = true;
	}
	
	mayFailPlacing = _node->get_bool_attribute_or_from_child_presence(TXT("mayFailPlacing"), mayFailPlacing);

	if (auto* attr = _node->get_attribute(TXT("doorDirectionObjectiveOverride")))
	{
		doorDirectionObjectiveOverride = DoorDirectionObjectiveOverride::parse(attr->get_as_string());
	}

	error_loading_xml_on_assert(sceneryType.is_name_valid(), result, _node, TXT("missing \"sceneryType\""));

	for_every(node, _node->children_named(TXT("poi")))
	{
		AI::Logics::Managers::SpawnManagerPOIDefinition poi;
		if (poi.load_from_xml(node))
		{
			poiDefinitions.push_back(poi);
		}
		else
		{
			result = false;
		}
	}

	slotOffset.load_from_xml_child_node(_node, TXT("slotOffset"));
	forMapLineModel.load_from_xml(_node, TXT("forMapLineModel"), _lc);

	deviceSetupVariables.load_from_xml_child_node(_node, TXT("deviceSetupVariables"));
	stateVariables.load_from_xml_child_node(_node, TXT("stateVariables"));

	for_every(node, _node->children_named(TXT("provides")))
	{
		if (auto* attr = node->get_attribute(TXT("energy")))
		{
			EnergyType::Type p = EnergyType::parse(attr->get_as_string());
			if (p != EnergyType::None)
			{
				providesEnergy[p] = true;
			}
		}
	}

	if (knownFromStart && !special)
	{
		warn_loading_xml(_node, TXT("if \"knownFromStart\", should be also \"special\""));
	}

	return result;
}

bool PilgrimageDevice::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= sceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= roomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= directionTexturePart.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	
	result &= forMapLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//
