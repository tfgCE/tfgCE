#include "doorType.h"

#include "door.h"
#include "doorInRoom.h"
#include "room.h"
#include "spaceBlocker.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#ifdef AN_CLANG
#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(requestedNominalDoorWidth);
DEFINE_STATIC_NAME(requestedNominalDoorHeight);

//

DoorOperation::Type DoorOperation::parse(String const & _string)
{
	if (_string == TXT("auto")) return DoorOperation::Auto;
	if (_string == TXT("autoEagerToClose")) return DoorOperation::AutoEagerToClose;
	if (_string == TXT("unclosable")) return DoorOperation::Unclosable;
	if (_string == TXT("stayOpen")) return DoorOperation::StayOpen;
	if (_string == TXT("open")) return DoorOperation::StayOpen;
	if (_string == TXT("stayClosed")) return DoorOperation::StayClosed;
	if (_string == TXT("close")) return DoorOperation::StayClosed;
	if (_string == TXT("closed")) return DoorOperation::StayClosed;
	error(TXT("door operation type \"%S\" not recognised"), _string.to_char());
	return DoorOperation::StayClosed;
}

//

bool DoorSounds::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	if (IO::XML::Node const* soundsNode = _node->first_child_named(TXT("sounds")))
	{
		if (IO::XML::Node const* node = soundsNode->first_child_named(TXT("open")))
		{
			result &= openSoundSample.load_from_xml(node, TXT("sample"), _lc);
		}
		if (IO::XML::Node const* node = soundsNode->first_child_named(TXT("opening")))
		{
			result &= openingSoundSample.load_from_xml(node, TXT("sample"), _lc);
		}
		if (IO::XML::Node const* node = soundsNode->first_child_named(TXT("opened")))
		{
			result &= openedSoundSample.load_from_xml(node, TXT("sample"), _lc);
		}
		if (IO::XML::Node const* node = soundsNode->first_child_named(TXT("close")))
		{
			result &= closeSoundSample.load_from_xml(node, TXT("sample"), _lc);
		}
		if (IO::XML::Node const* node = soundsNode->first_child_named(TXT("closing")))
		{
			result &= closingSoundSample.load_from_xml(node, TXT("sample"), _lc);
		}
		if (IO::XML::Node const* node = soundsNode->first_child_named(TXT("closed")))
		{
			result &= closedSoundSample.load_from_xml(node, TXT("sample"), _lc);
		}
	}

	return result;
}

bool DoorSounds::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= openSoundSample.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= openingSoundSample.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= openedSoundSample.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= closeSoundSample.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= closingSoundSample.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= closedSoundSample.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

void DoorSounds::load_data_on_demand_if_required()
{
	if (auto* s = openSoundSample.get()) s->load_on_demand_if_required();
	if (auto* s = openingSoundSample.get()) s->load_on_demand_if_required();
	if (auto* s = openedSoundSample.get()) s->load_on_demand_if_required();
	if (auto* s = closeSoundSample.get()) s->load_on_demand_if_required();
	if (auto* s = closingSoundSample.get()) s->load_on_demand_if_required();
	if (auto* s = closedSoundSample.get()) s->load_on_demand_if_required();
}

//

DoorWingType::DoorWingType()
: placement(Transform::identity)
, slideOffset(Vector3::zero)
, meshGeneratorRequestedNominalDoorWidthParamName(NAME(requestedNominalDoorWidth))
, meshGeneratorRequestedNominalDoorHeightParamName(NAME(requestedNominalDoorHeight))
{
	SET_EXTRA_DEBUG_INFO(autoClipPlanes, TXT("DoorWingType.autoClipPlanes"));
}

bool DoorWingType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, Array<DoorWingType> const& _wings)
{
	bool result = true;
	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	result &= mesh.load_from_xml(_node, TXT("mesh"), _lc);
	result &= meshGenerator.load_from_xml(_node, TXT("meshGenerator"), _lc);
	meshGeneratorRequestedNominalDoorWidthParamName = _node->get_name_attribute(TXT("generatorRequestedNominalDoorWidthParamName"), meshGeneratorRequestedNominalDoorWidthParamName);
	meshGeneratorRequestedNominalDoorHeightParamName = _node->get_name_attribute(TXT("generatorRequestedNominalDoorHeightParamName"), meshGeneratorRequestedNominalDoorHeightParamName);
	asymmetrical = _node->get_bool_attribute_or_from_child_presence(TXT("asymmetrical"), asymmetrical);
	placement.load_from_xml_child_node(_node, TXT("placement"));
	slideOffset.load_from_xml_child_node(_node, TXT("slideOffset"));
	mapOpenPt.load_from_xml_child_node(_node, TXT("mapOpenPt"));
	nominalDoorSize.load_from_xml(_node, TXT("nominalDoorSize"));
	for_every(cn, _node->children_named(TXT("clipPlanes")))
	{
		Name copyClipPlanesFromId;
		copyClipPlanesFromId = cn->get_name_attribute_or_from_child(TXT("useOf"), copyClipPlanesFromId);
		copyClipPlanesFromId = cn->get_name_attribute_or_from_child(TXT("copyFrom"), copyClipPlanesFromId);
		if (copyClipPlanesFromId.is_valid())
		{
			for_every(w, _wings)
			{
				if (w->id == copyClipPlanesFromId && copyClipPlanesFromId != id)
				{
					clipPlanes = w->clipPlanes;
				}
			}
		}
	}
	clipPlanes.load_from_xml_child_nodes(_node, TXT("clipPlane"));
	autoClipPlanes.clear();
	for_every(node, _node->children_named(TXT("autoClipPlane")))
	{
		Name meshNode = node->get_name_attribute(TXT("meshNode"));
		if (meshNode.is_valid())
		{
			autoClipPlanes.push_back(meshNode);
		}
	}
	for_every(cn, _node->children_named(TXT("sounds")))
	{
		Name copySoundsFromId;
		copySoundsFromId = cn->get_name_attribute_or_from_child(TXT("useOf"), copySoundsFromId);
		copySoundsFromId = cn->get_name_attribute_or_from_child(TXT("copyFrom"), copySoundsFromId);
		if (copySoundsFromId.is_valid())
		{
			for_every(w, _wings)
			{
				if (w->id == copySoundsFromId && copySoundsFromId != id)
				{
					sounds = w->sounds;
				}
			}
		}
	}
	sounds.load_from_xml(_node, _lc);
	return result;
}

bool DoorWingType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	if (mesh.is_name_valid())
	{
		result &= mesh.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (meshGenerator.is_name_valid())
	{
		result &= meshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	result &= sounds.prepare_for_game(_library, _pfgContext);
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::GenerateMeshes)
	{
		result &= generate_mesh(_library);
	}
	return result;
}

void DoorWingType::load_data_on_demand_if_required()
{
	sounds.load_data_on_demand_if_required();
}

Mesh* DoorWingType::get_mesh_for(DoorInRoom const* _doorInRoom) const
{
	if (mesh.is_set())
	{
		return mesh.get();
	}

	if (!meshGenerator.is_set())
	{
		return nullptr;
	}

	GeneratedMeshDeterminants determinants;

	an_assert(doorType);

	determinants.gather_for(_doorInRoom, doorType->generatedMeshesDeterminants);

	if (auto* mesh = generatedMeshes.get_mesh_for(determinants))
	{
		return mesh;
	}

	if (auto* mesh = generatedMeshes.generate_mesh_for(determinants, meshGenerator.get(), doorType, meshGeneratorRequestedNominalDoorWidthParamName, meshGeneratorRequestedNominalDoorHeightParamName))
	{
		return mesh;
	}

	return nullptr;
}

bool DoorWingType::generate_mesh(Library* _library)
{
	bool result = true;
	if (meshGenerator.get())
	{
		an_assert(doorType);
		generatedMeshes.generate_all_meshes_for(meshGenerator.get(), doorType, meshGeneratorRequestedNominalDoorWidthParamName, meshGeneratorRequestedNominalDoorHeightParamName);
	}
	return result;
}

//

REGISTER_FOR_FAST_CAST(DoorType);
LIBRARY_STORED_DEFINE_TYPE(DoorType, doorType);

void DoorType::initialise_static()
{
}

void DoorType::close_static()
{
}

DoorType::DoorType(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, frameMeshGeneratorRequestedNominalDoorWidthParamName(NAME(requestedNominalDoorWidth))
, frameMeshGeneratorRequestedNominalDoorHeightParamName(NAME(requestedNominalDoorHeight))
{
	regionConnector.def.doorType.lock(this);
}

void DoorType::prepare_to_unload()
{
	base::prepare_to_unload();
	regionConnector.def.doorType.unlock_and_clear();
	if (ownRoomPart.get())
	{
		ownRoomPart->prepare_to_unload();
	}
}

DoorType::~DoorType()
{
}

bool DoorType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	heightFromWidth = _node->get_bool_attribute_or_from_child_presence(TXT("heightFromWidth"), heightFromWidth);

	fakeOneWayWindow = _node->get_bool_attribute_or_from_child_presence(TXT("fakeOneWayWindow"), fakeOneWayWindow);
	
	relevantForMovement = _node->get_bool_attribute_or_from_child_presence(TXT("relevantForMovement"), relevantForMovement);
	relevantForMovement = ! _node->get_bool_attribute_or_from_child_presence(TXT("ignoreForMovement"), ! relevantForMovement);
	relevantForVR = ! _node->get_bool_attribute_or_from_child_presence(TXT("relevantForVR"), !relevantForVR);
	relevantForVR = ! _node->get_bool_attribute_or_from_child_presence(TXT("ignoreForVR"), !relevantForVR);

	doorHoleShaderProgram.load_from_xml(_node, TXT("doorHoleShaderProgram"), _lc);

	if (IO::XML::Node const* node = _node->first_child_named(TXT("compatibleSize")))
	{
		compatibleSize.load_from_xml(node);

		if (!compatibleSize.is_empty())
		{
			if (compatibleSize.x.min == compatibleSize.x.max)
			{
				float w = compatibleSize.x.max;
				float hw = w * 0.5f;
				compatibleSize.x.min = -hw;
				compatibleSize.x.max =  hw;
			}
			if (compatibleSize.y.min == compatibleSize.y.max)
			{
				compatibleSize.y.min = 0.0f;
			}
		}
	}

	hole.set_height_from_width(heightFromWidth);

	if (IO::XML::Node const * holeNode = _node->first_child_named(TXT("hole")))
	{
		result &= hole.load_from_xml(holeNode);
		doorHoleShaderProgram.load_from_xml(holeNode, TXT("shaderProgram"), _lc);
	}
	else
	{
		error_loading_xml(_node, TXT("no \"hole\" sub node"));
		result = false;
	}

	roomPart.load_from_xml(_node, TXT("roomPart"), _lc);

	sounds.load_from_xml(_node, _lc);

	result &= generatedMeshesDeterminants.load_from_xml_child_node(_node, TXT("meshesDeterminants"));
	result &= generatedMeshesDeterminants.load_from_xml_child_node(_node, TXT("generatedMeshesDeterminants"));

	result &= meshGeneratorParameters.load_from_xml_child_node(_node, TXT("meshGeneratorParameters"));

	if (IO::XML::Node const * frameMeshNode = _node->first_child_named(TXT("frameMesh")))
	{
		result &= frameMesh.load_from_xml(frameMeshNode, TXT("mesh"), _lc);
		result &= frameMeshGenerator.load_from_xml(frameMeshNode, TXT("meshGenerator"), _lc);
		frameMeshGeneratorRequestedNominalDoorWidthParamName = frameMeshNode->get_name_attribute(TXT("generatorRequestedNominalDoorWidthParamName"), frameMeshGeneratorRequestedNominalDoorWidthParamName);
		frameMeshGeneratorRequestedNominalDoorHeightParamName = frameMeshNode->get_name_attribute(TXT("generatorRequestedNominalDoorHeightParamName"), frameMeshGeneratorRequestedNominalDoorHeightParamName);
		frameMeshPlacement.load_from_xml(frameMeshNode);
		if (IO::XML::Attribute const * attr = frameMeshNode->get_attribute(TXT("type")))
		{
			if (attr->get_as_string() == TXT("half") ||
				attr->get_as_string() == TXT("oneSide"))
			{
				halfFrame = true;
			}
			else if (attr->get_as_string() == TXT("full"))
			{
				halfFrame = false;
			}
			else
			{
				error_loading_xml(frameMeshNode, TXT("frame type \"%S\" not recognised"), attr->get_as_string().to_char());
				result = false;
			}
		}
	}

	canSeeThroughWhenClosed = _node->get_bool_attribute(TXT("canSeeThroughWhenClosed"), canSeeThroughWhenClosed);
	canOpenInTwoDirections = _node->get_bool_attribute(TXT("canOpenInTwoDirections"), canOpenInTwoDirections);
	
	if (IO::XML::Node const * node = _node->first_child_named(TXT("soundHearable")))
	{
		result &= soundHearableWhenOpen.load_from_xml(node, TXT("sound"), TXT("open"));
	}
	ignoreSoundDistanceBeyondFirstRoom = _node->get_bool_attribute(TXT("ignoreSoundDistanceBeyondFirstRoom"), ignoreSoundDistanceBeyondFirstRoom);

	if (IO::XML::Node const * doorWingNodes = _node->first_child_named(TXT("doorWings")))
	{
		wingsOpeningTime = doorWingNodes->get_float_attribute(TXT("operatingTime"), wingsOpeningTime);
		wingsClosingTime = doorWingNodes->get_float_attribute(TXT("operatingTime"), wingsClosingTime);
		wingsOpeningTime = doorWingNodes->get_float_attribute(TXT("openingTime"), wingsOpeningTime);
		wingsClosingTime = doorWingNodes->get_float_attribute(TXT("closingTime"), wingsClosingTime);
		for_every(doorWingNode, doorWingNodes->children_named(TXT("doorWing")))
		{
			DoorWingType doorWing;
			doorWing.doorType = this;
			if (doorWing.load_from_xml(doorWingNode, _lc, wings))
			{
				wings.push_back(doorWing);
			}
			else
			{
				error(TXT("error loading door wing"));
				result = false;
			}
		}
	}

	canBeOffFloor = _node->get_bool_attribute_or_from_child_presence(TXT("canBeOffFloor"), canBeOffFloor);

	isNavConnector = _node->get_bool_attribute_or_from_child_presence(TXT("isNavConnector"), isNavConnector);
	isNavConnector = !_node->get_bool_attribute_or_from_child_presence(TXT("notNavConnector"), ! isNavConnector);

	replacableByDoorTypeReplacer = _node->get_bool_attribute_or_from_child_presence(TXT("replacableByDoorTypeReplacer"), replacableByDoorTypeReplacer);
	replacableByDoorTypeReplacer = !_node->get_bool_attribute_or_from_child_presence(TXT("notReplacableByDoorTypeReplacer"), !replacableByDoorTypeReplacer);

	if (IO::XML::Node const * roomPartNode = _node->first_child_named(TXT("doorRoomPartWithSingleConnector")))
	{
		// this is simplified version of room part that assumes that connector is placed in point 0,0,0 facing yaw 0 and is always required
		roomPart.set(nullptr);
		ownRoomPart = new RoomPart(nullptr, get_name()); // keep same name, even if this won't be put into library
		ownRoomPart->get_as_room_piece()->def.canBeUsedForPlacementInRoom = false; // shouldn't be used for placement
		PieceCombiner::Connector<RoomPieceCombinerGenerator> doorConnector(PieceCombiner::ConnectorType::Normal);
		Vector3 linkLocationOffset = Vector3::zero;
		linkLocationOffset.load_from_xml_child_node(roomPartNode, TXT("linkLocationOffset"));
		doorConnector.def.placement = Transform(linkLocationOffset, Rotator3::zero.to_quat());
		doorConnector.requiredCount = Random::Number<int>(1);
		doorConnector.priority = -1; // below normal, but not extremely low
		result &= doorConnector.load_from_xml(roomPartNode);
		ownRoomPart->get_as_room_piece()->connectors.push_back(doorConnector);
	}
	else if (IO::XML::Node const * roomPartNode = _node->first_child_named(TXT("roomPart")))
	{
		roomPart.set(nullptr);
		ownRoomPart = new RoomPart(nullptr, get_name()); // keep same name, even if this won't be put into library
		ownRoomPart->get_as_room_piece()->def.canBeUsedForPlacementInRoom = false; // shouldn't be used for placement
		result &= ownRoomPart->load_from_xml(roomPartNode, _lc);
	}

	if (wings.is_empty())
	{
		operation = DoorOperation::StayOpen;
	}
	else
	{
		canBeClosed = true;
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("operation")))
	{
		operation = DoorOperation::parse(attr->get_as_string());
	}
	canBeClosed = _node->get_bool_attribute_or_from_child_presence(TXT("canBeClosed"), canBeClosed);
	canBeClosed = ! _node->get_bool_attribute_or_from_child_presence(TXT("cantBeClosed"), ! canBeClosed);
	if (operation == DoorOperation::Auto ||
		operation == DoorOperation::StayClosed ||
		operation == DoorOperation::StayClosedTemporarily ||
		operation == DoorOperation::StayOpen)
	{
		if (!canBeClosed)
		{
			warn_loading_xml_dev_ignore(_node, TXT("sort out door operation (unclosable?) or explicitly tell that door can be closed"));
			canBeClosed = true;
		}
	}
	autoCloseDelay.load_from_xml(_node, TXT("autoCloseDelay"));
	autoOperationDistance = _node->get_float_attribute_or_from_child(TXT("autoOperationDistance"), autoOperationDistance);
	autoOperationTagCondition.load_from_xml_attribute_or_child_node(_node, TXT("autoOperationTagCondition"));

	result &= regionConnector.load_from_xml(_node->first_child_named(TXT("asRegionConnector")), RegionGenerator::LoadingContext(_lc).temp_ptr());

	if (IO::XML::Node const * blocksEnvironmentPropagationNode = _node->first_child_named(TXT("blocksEnvironmentPropagation")))
	{
		blocksEnvironmentPropagation = true;
	}

	for_every(node, _node->children_named(TXT("vrDoorWidth")))
	{
		vrDoorWidth.nominal = node->get_float_attribute_or_from_child(TXT("nominal"), vrDoorWidth.nominal);
		vrDoorWidth.width = node->get_float_attribute_or_from_child(TXT("width"), vrDoorWidth.width);
		vrDoorWidth.add = node->get_float_attribute_or_from_child(TXT("add"), vrDoorWidth.add);
	}

	result &= doorVRCorridor.load_from_xml(_node, _lc);

	withSpaceBlockers = _node->get_bool_attribute_or_from_child_presence(TXT("withSpaceBlockers"), withSpaceBlockers);
	withSpaceBlockers = ! _node->get_bool_attribute_or_from_child_presence(TXT("noSpaceBlockers"), ! withSpaceBlockers);
	spaceBlockerSmallerRequiresToBeEmpty = _node->get_bool_attribute_or_from_child_presence(TXT("spaceBlockerSmallerRequiresToBeEmpty"), spaceBlockerSmallerRequiresToBeEmpty);
	spaceBlockerExtraWidth = _node->get_float_attribute_or_from_child(TXT("spaceBlockerExtraWidth"), spaceBlockerExtraWidth);

	forSpawnedThroughPOI.clear();
	for_every(containerNode, _node->children_named(TXT("forSpawnedThroughPOI")))
	{
		for_every(node, containerNode->children_named(TXT("for")))
		{
			ForSpawnedThroughPOI fstp;
			fstp.chance = node->get_float_attribute(TXT("chance"), fstp.chance);
			fstp.roomTagged.load_from_xml_attribute(node, TXT("roomTagged"));
			fstp.useDoorType.load_from_xml(node, TXT("useDoorType"), _lc);
			error_loading_xml_on_assert(
				fstp.chance < 1.0f ||
				!fstp.roomTagged.is_empty(),
				result, node, TXT("define conditions for forSpawnedThroughPOI.for"));
			error_loading_xml_on_assert(
				fstp.useDoorType.is_name_valid(),
				result, node, TXT("define useDoorType for forSpawnedThroughPOI.for"));
			forSpawnedThroughPOI.push_back(fstp);
		}
	}

	return result;
}

bool DoorType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);

	if (ownRoomPart.get())
	{
		ownRoomPart->set_library(_library);
		result &= ownRoomPart->prepare_for_game(_library, _pfgContext);
		roomPart = ownRoomPart.get();
	}
	result &= frameMesh.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= frameMeshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= sounds.prepare_for_game(_library, _pfgContext);
	for_every(wing, wings)
	{
		result &= wing->prepare_for_game(_library, _pfgContext);
	}

	result &= doorVRCorridor.prepare_for_game(_library, _pfgContext);

	result &= doorHoleShaderProgram.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	for_every(fstp, forSpawnedThroughPOI)
	{
		fstp->useDoorType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		result &= regionConnector.prepare(nullptr, RegionGenerator::PreparingContext(_library).temp_ptr());

		if (roomPart.is_name_valid() && !ownRoomPart.get())
		{
			roomPart.find(_library);
		}
		if (roomPart.get() == nullptr)
		{
			//error(TXT("room part not defined for door type '%S'"), get_name().to_string().to_char());
			//result = false;
		}

		if (roomPart.get())
		{
			result &= roomPart->get_as_room_piece()->has_connectors();
		}

		if (!wings.is_empty())
		{
			if (wingsOpeningTime == 0.0f)
			{
				todo_future(TXT("get from animation"));
			}
			if (wingsClosingTime == 0.0f)
			{
				todo_future(TXT("get from animation"));
			}
			if (wingsClosingTime == 0.0f)
			{
				wingsClosingTime = wingsOpeningTime;
			}
		}

		if (soundHearableWhenOpen.is_empty())
		{
			soundHearableWhenOpen.add(0.0f, 0.3f); // by default it should be hearable a little bit through door
			soundHearableWhenOpen.add(0.1f, 0.9f); // when just open a little make it almost fully hearable
			soundHearableWhenOpen.add(1.0f, 1.0f);
		}
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::GenerateMeshes)
	{
		result &= generate_mesh(_library);
	}

	return result;
}

void DoorType::load_data_on_demand_if_required()
{
	sounds.load_data_on_demand_if_required();
	for_every(w, wings)
	{
		w->load_data_on_demand_if_required();
	}
}

float DoorType::calculate_vr_width() const
{
	float scaleWidth = 1.0f;
	if (Door::get_nominal_door_width().is_set() &&
		vrDoorWidth.nominal != 0.0f)
	{
		scaleWidth = Door::get_nominal_door_width().get() / vrDoorWidth.nominal;
	}
	float result = vrDoorWidth.width * scaleWidth + vrDoorWidth.add;

	if (result == 0.0f)
	{
		result = get_hole()->calculate_size(DoorSide::A, Vector2::one).x.length() * 1.01f;
	}

	return result;
}

void DoorType::generate_meshes()
{
	hole.scale_to_nominal_door_size_and_build();

	generate_mesh(Library::get_current());

	for_every(wing, wings)
	{
		wing->generate_mesh(Library::get_current());
	}
}

void DoorType::generate_all_meshes()
{
	if (auto* library = Library::get_current())
	{
		library->get_door_types().do_for_every(
			[](Framework::LibraryStored* _libraryStored)
			{
				if (Framework::DoorType* dt = fast_cast<Framework::DoorType>(_libraryStored))
				{
					dt->generate_meshes();
				}
			});
	}
}

bool DoorType::generate_mesh(Library* _library)
{
	bool result = true;
	if (frameMeshGenerator.get())
	{
		generatedFrameMeshes.generate_all_meshes_for(frameMeshGenerator.get(), this, frameMeshGeneratorRequestedNominalDoorWidthParamName, frameMeshGeneratorRequestedNominalDoorHeightParamName);
	}
	return result;
}

Mesh const* DoorType::get_frame_mesh_for(DoorInRoom const* _doorInRoom, GeneratedMeshDeterminants const& _determinants) const
{
	if (frameMesh.is_set())
	{
		return frameMesh.get();
	}

	if (!frameMeshGenerator.is_set())
	{
		return nullptr;
	}

	if (auto* mesh = generatedFrameMeshes.get_mesh_for(_determinants))
	{
		return mesh;
	}

	if (auto* mesh = generatedFrameMeshes.generate_mesh_for(_determinants, frameMeshGenerator.get(), this, frameMeshGeneratorRequestedNominalDoorWidthParamName, frameMeshGeneratorRequestedNominalDoorHeightParamName))
	{
		return mesh;
	}
	return nullptr;
}

Range2 DoorType::calculate_size(Optional<DoorSide::Type> const & _side, Optional<Vector2> const& _scaleDoorSize) const
{
	return get_hole()? get_hole()->calculate_size(_side.get(DoorSide::Any), _scaleDoorSize.get(Vector2::one)) : Range2(Range(0.0f), Range(0.0f));
}

Range2 DoorType::calculate_compatible_size(Optional<DoorSide::Type> const & _side, Optional<Vector2> const& _scaleDoorSize) const
{
	Range2 result = Range2::empty;
	result = compatibleSize;
	if (!result.is_empty())
	{
		if (_side.is_set() && _side.get() == DoorSide::B)
		{
			swap(result.x.min, result.x.max);
			result.x.min = -result.x.min;
			result.x.max = -result.x.max;
		}
		if (_scaleDoorSize.is_set())
		{
			result.x.min *= _scaleDoorSize.get().x;
			result.x.max *= _scaleDoorSize.get().x;
			result.y.min *= _scaleDoorSize.get().y;
			result.y.max *= _scaleDoorSize.get().y;
		}
	}
	return result;
}

void DoorType::create_space_blocker(REF_ SpaceBlockers& _spaceBlockers, Optional<Transform> const& _at, Optional<DoorSide::Type> const& _side, Optional<Vector2> const& _scaleDoorSize) const
{
	if (!does_have_space_blockers())
	{
		return;
	}

	Transform at = _at.get(Transform::identity);

	Range2 size = calculate_size(_side, _scaleDoorSize);

	float doorThickness = 0.02f;
	{
		Vector3 boxAt = Vector3(size.x.centre(), -doorThickness * 0.5f, size.y.centre());
		Vector3 boxSize = Vector3(size.x.length() + spaceBlockerExtraWidth, doorThickness, size.y.length());

		SpaceBlocker sb;
		sb.blocks = true;
		sb.requiresToBeEmpty = true;
		sb.box.setup(at.location_to_world(boxAt), boxSize * 0.5f,
			at.vector_to_world(Vector3::xAxis),
			at.vector_to_world(Vector3::yAxis));

#ifdef AN_DEVELOPMENT
		sb.debugInfo = TXT("door back");
#endif

		_spaceBlockers.blockers.push_back(sb);
	}
	{
		Vector3 boxSize = Vector3(size.x.length() + spaceBlockerExtraWidth - doorThickness * 4.0f, doorThickness * 2.0f, size.y.length());
		Vector3 boxAt = Vector3(size.x.centre(), -doorThickness - boxSize.y * 0.5f, size.y.centre());

		SpaceBlocker sb;
		sb.blocks = true;
		sb.requiresToBeEmpty = true;
		sb.box.setup(at.location_to_world(boxAt), boxSize * 0.5f,
			at.vector_to_world(Vector3::xAxis),
			at.vector_to_world(Vector3::yAxis));

#ifdef AN_DEVELOPMENT
		sb.debugInfo = TXT("door front near");
#endif

		_spaceBlockers.blockers.push_back(sb);
	}

	{
		float spaceThickness = size.x.length() * 0.8f;
		// leave some space above, so we can place turrets etc
		Range height = size.y;
		height.max = max(height.max - 0.3f, height.get_at(0.75f));

		if (is_space_blocker_smaller_for_requires_to_be_empty())
		{
			spaceThickness = min(0.1f, spaceThickness);
		}
		
		Vector3 boxAt = Vector3(size.x.centre(), -spaceThickness * 0.5f - doorThickness * 0.5f, height.centre());
		Vector3 boxSize = Vector3(size.x.length() - 0.05f, spaceThickness - doorThickness, height.length());

		// always make them bit more narrow to allow items on sides to fit
		boxSize.x = max(0.2f, boxSize.x * 0.8f);
		if (is_space_blocker_smaller_for_requires_to_be_empty())
		{
			boxSize.x = max(0.2f, boxSize.x * 0.65f);
		}

		SpaceBlocker sb;
		sb.blocks = false;
		sb.requiresToBeEmpty = true;
		sb.box.setup(at.location_to_world(boxAt), boxSize * 0.5f,
			at.vector_to_world(Vector3::xAxis),
			at.vector_to_world(Vector3::yAxis));

#ifdef AN_DEVELOPMENT
		sb.debugInfo = TXT("door empty");
#endif

		_spaceBlockers.blockers.push_back(sb);
	}
}

DoorType* DoorType::process_for_spawning_through_poi(Room const* _forInRoom, Random::Generator& _rg)
{
	DoorType* dt = this;
	bool shouldContinue = true;
	while (shouldContinue)
	{
		shouldContinue = false;
		for_every(fstp, dt->forSpawnedThroughPOI)
		{
			if (fstp->chance < 1.0f)
			{
				if (!_rg.get_chance(fstp->chance))
				{
					continue;
				}
			}
			if (! fstp->roomTagged.is_empty() && _forInRoom)
			{
				if (!fstp->roomTagged.check(_forInRoom->get_tags()))
				{
					continue;
				}
			}
			if (auto* nextdt = fstp->useDoorType.get())
			{
				dt = nextdt;
				shouldContinue = true;
				// jump to next dt
				break;
			}
		}
	}
	return dt;
}

//

Mesh* DoorGeneratedMeshSet::get_mesh_for(GeneratedMeshDeterminants const& _determinants) const
{
	Concurrency::ScopedMRSWLockRead lock(meshesLock);

	for_every(m, meshes)
	{
		if (m->forDeterminants == _determinants)
		{
			return m->mesh.get();
		}
	}

	return nullptr;
}

Mesh* DoorGeneratedMeshSet::generate_mesh_for(GeneratedMeshDeterminants const& _determinants, MeshGenerator* _meshGenerator, DoorType const* _doorType, Name const& meshGeneratorRequestedNominalDoorWidthParamName, Name const& meshGeneratorRequestedNominalDoorHeightParamName)
{
	Concurrency::ScopedMRSWLockWrite lock(meshesLock);
	for_every(m, meshes)
	{
		if (m->forDeterminants == _determinants)
		{
			m->generate_mesh_for(_meshGenerator, _doorType, meshGeneratorRequestedNominalDoorWidthParamName, meshGeneratorRequestedNominalDoorHeightParamName);
			return m->mesh.get();
		}
	}

	meshes.push_back(DoorGeneratedMesh());
	auto& m = meshes.get_last();
	m.forDeterminants = _determinants;
	m.generate_mesh_for(_meshGenerator, _doorType, meshGeneratorRequestedNominalDoorWidthParamName, meshGeneratorRequestedNominalDoorHeightParamName);
	return m.mesh.get();
}

void DoorGeneratedMeshSet::generate_all_meshes_for(MeshGenerator* _meshGenerator, DoorType const* _doorType, Name const& meshGeneratorRequestedNominalDoorWidthParamName, Name const& meshGeneratorRequestedNominalDoorHeightParamName)
{
	Concurrency::ScopedMRSWLockWrite lock(meshesLock);
	if (meshes.is_empty())
	{
		meshes.push_back(DoorGeneratedMesh());
		meshes.get_last().forDeterminants = _doorType->generatedMeshesDeterminants;
	}
	for_every(m, meshes)
	{
		m->generate_mesh_for(_meshGenerator, _doorType, meshGeneratorRequestedNominalDoorWidthParamName, meshGeneratorRequestedNominalDoorHeightParamName);
	}
}

//

int fake_hash_to_int(tchar const* _text)
{
	int ret = 0;
	while (*_text != 0)
	{
		ret += (int)(*_text);
		++_text;
	}

	return ret;
}

void DoorGeneratedMesh::generate_mesh_for(MeshGenerator* _meshGenerator, DoorType const* doorType, Name const& meshGeneratorRequestedNominalDoorWidthParamName, Name const& meshGeneratorRequestedNominalDoorHeightParamName)
{
	SimpleVariableStorage meshGeneratorParametersLocal = doorType->get_mesh_generator_parameters();
	if (Door::get_nominal_door_width().is_set() &&
		meshGeneratorRequestedNominalDoorWidthParamName.is_valid())
	{
		meshGeneratorParametersLocal.access<float>(meshGeneratorRequestedNominalDoorWidthParamName) = Door::get_nominal_door_width().get();
	}
	if (meshGeneratorRequestedNominalDoorHeightParamName.is_valid())
	{
		if (doorType && doorType->should_use_height_from_width())
		{
			if (Door::get_nominal_door_width().is_set())
			{
				meshGeneratorParametersLocal.access<float>(meshGeneratorRequestedNominalDoorHeightParamName) = Door::get_nominal_door_width().get();
			}
		}
		else
		{
			meshGeneratorParametersLocal.access<float>(meshGeneratorRequestedNominalDoorHeightParamName) = Door::get_nominal_door_height();
		}
	}
	forDeterminants.apply_to(meshGeneratorParametersLocal);
	mesh = _meshGenerator->generate_temporary_library_mesh(Library::get_current(),
		MeshGeneratorRequest().using_parameters(meshGeneratorParametersLocal)
							  .no_lods()
							  .using_random_regenerator(Random::Generator(fake_hash_to_int(doorType->get_name().get_name().to_char()), 0)));
	if (!mesh.is_set())
	{
		error(TXT("could not generate wing mesh for \"%S\""), doorType->get_name().to_string().to_char());
	}
}
