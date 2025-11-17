#include "roomGenerator_roomPieceCombinerInfo.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"

#include "..\..\..\core\pieceCombiner\pieceCombinerImplementation.h"

#include "..\doorInRoom.h"
#include "..\room.h"

using namespace Framework;
using namespace RoomGenerators;

//

bool RoomPieceCombinerInfo::Set::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	chance = _node->get_float_attribute(TXT("chance"), chance);

	for_every(includeInstanceNode, _node->children_named(TXT("includeInstance")))
	{
		if (includeInstanceNode->has_attribute(TXT("roomPart")))
		{
			UsedLibraryStored<RoomPart> includeInstanceRoomPart;
			if (includeInstanceRoomPart.load_from_xml(includeInstanceNode, TXT("roomPart"), _lc))
			{
				includeInstanceRoomParts.push_back(includeInstanceRoomPart);
			}
			else
			{
				error_loading_xml(includeInstanceNode, TXT("invalid room part @ %S"), includeInstanceNode->get_location_info().to_char());
				result = false;
			}
		}
	}

	return result;
}

bool RoomPieceCombinerInfo::Set::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(includeInstanceRoomPart, includeInstanceRoomParts)
	{
		result &= includeInstanceRoomPart->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

bool RoomPieceCombinerInfo::Set::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	for_every(includeInstanceRoomPart, includeInstanceRoomParts)
	{
		result &= _renamer.apply_to(*includeInstanceRoomPart, _library);
	}

	return result;
}

//

bool RoomPieceCombinerInfo::DoorOverride::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	outerName = _node->get_name_attribute(TXT("outer"), outerName);
	result &= outerName.is_valid();
	result &= roomPart.load_from_xml(_node, TXT("roomPart"), _lc);
	return result;
}

bool RoomPieceCombinerInfo::DoorOverride::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= roomPart.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return true;
}

bool RoomPieceCombinerInfo::DoorOverride::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _renamer.apply_to(roomPart, _library);

	return result;
}

//

REGISTER_FOR_FAST_CAST(RoomPieceCombinerInfo);

RoomPieceCombinerInfo::RoomPieceCombinerInfo()
{
}

RoomPieceCombinerInfo::~RoomPieceCombinerInfo()
{
}

bool RoomPieceCombinerInfo::load_room_generation_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(doorOverrideNode, _node->children_named(TXT("doorOverride")))
	{
		DoorOverride doorOverride;
		if (doorOverride.load_from_xml(doorOverrideNode, _lc))
		{
			doorOverrides.push_back(doorOverride);
		}
		else
		{
			todo_important(TXT("error"));
			result = false;
		}
	}

	for_every(addNode, _node->children_named(TXT("include")))
	{
		if (IO::XML::Attribute const * attr = addNode->get_attribute(TXT("roomPart")))
		{
			UsedLibraryStored<RoomPart> roomPart;
			if (roomPart.load_from_xml(addNode, TXT("roomPart"), _lc))
			{
				includedRoomParts.push_back(roomPart);
			}
			else
			{
				todo_important(TXT("error"));
				result = false;
			}
		}
	}

	for_every(setNode, _node->children_named(TXT("obligatorySet")))
	{
		obligatorySets.push_back(Set());
		if (!obligatorySets.get_last().load_from_xml(setNode, _lc))
		{
			obligatorySets.pop_back();
			result = false;
		}
	}

	for_every(setNode, _node->children_named(TXT("set")))
	{
		sets.push_back(Set());
		if (!sets.get_last().load_from_xml(setNode, _lc))
		{
			sets.pop_back();
			result = false;
		}
	}

	return result;
}

bool RoomPieceCombinerInfo::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (IO::XML::Node const * generationRulesNode = _node->first_child_named(TXT("generationRules")))
	{
		result &= generationRules.load_from_xml(generationRulesNode);
	}

	result &= load_room_generation_from_xml(_node, _lc);

	for_every(includeTaggedNode, _node->children_named(TXT("includeTagged")))
	{
		if (IO::XML::Attribute const * attr = includeTaggedNode->get_attribute(TXT("roomPart")))
		{
			TagCondition* tagCondition = new TagCondition();
			if (tagCondition->load_from_string(attr->get_value(), TagConditionOperator::And))
			{
				includedRoomPartsTagConditions.push_back(RefCountObjectPtr<TagCondition>(tagCondition));
			}
			else
			{
				todo_important(TXT("error"));
				result = false;
				delete tagCondition;
			}
		}
	}

	RoomPieceCombinerGenerator::LoadingContext loadingContext(_lc);

	for_every(pieceNode, _node->children_named(TXT("roomPart")))
	{
		RefCountObjectPtr<PieceCombiner::Piece<RoomPieceCombinerGenerator>> piece( new PieceCombiner::Piece<RoomPieceCombinerGenerator>() );
		bool loaded = piece->load_from_xml(pieceNode, &loadingContext);
		result &= loaded;
		if (loaded)
		{
			roomPieces.push_back(piece);
		}
	}

	for_every(outerConnectorNode, _node->children_named(TXT("outerRoomConnector")))
	{
		RefCountObjectPtr<PieceCombiner::Connector<RoomPieceCombinerGenerator>> outerConnector( new PieceCombiner::Connector<RoomPieceCombinerGenerator>() );
		bool loaded = outerConnector->load_from_xml(outerConnectorNode, &loadingContext);
		if (loaded && !outerConnector->outerName.is_valid())
		{
			error_loading_xml(outerConnectorNode, TXT("no outer defined for outer connector"));
			loaded = false;;
		}
		result &= loaded;
		if (loaded)
		{
			outerRoomConnectors.push_back(outerConnector);
		}
	}

	return result;
}

bool RoomPieceCombinerInfo::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(set, obligatorySets)
	{
		result &= set->prepare_for_game(_library, _pfgContext);
	}

	for_every(set, sets)
	{
		result &= set->prepare_for_game(_library, _pfgContext);
	}

	for_every(doorOverride, doorOverrides)
	{
		result &= doorOverride->prepare_for_game(_library, _pfgContext);
	}

	for_every(includedRoomPart, includedRoomParts)
	{
		result &= includedRoomPart->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		RoomPieceCombinerGenerator::PreparingContext preparingContext(_library);

		for_every_ref(piece, roomPieces)
		{
			result &= piece->prepare(&preparingContext);
		}

		for_every_ref(outerConnector, outerRoomConnectors)
		{
			result &= outerConnector->prepare(nullptr, &preparingContext);
		}
	}

	return result;
}

bool RoomPieceCombinerInfo::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	for_every_ref(roomPiece, roomPieces)
	{
		result &= RoomPieceCombinerGenerator::apply_renamer_to(*roomPiece, _renamer, _library);
	}

	for_every_ref(outerRoomConnector, outerRoomConnectors)
	{
		result &= RoomPieceCombinerGenerator::apply_renamer_to(*outerRoomConnector, _renamer, _library);
	}

	for_every(includedRoomPart, includedRoomParts)
	{
		result &= _renamer.apply_to(*includedRoomPart,  _library);
	}

	for_every(set, obligatorySets)
	{
		result &= set->apply_renamer(_renamer, _library);
	}

	for_every(set, sets)
	{
		result &= set->apply_renamer(_renamer, _library);
	}

	for_every(doorOverride, doorOverrides)
	{
		result &= doorOverride->apply_renamer(_renamer, _library);
	}

	return result;
}

RoomGeneratorInfoPtr RoomPieceCombinerInfo::create_copy() const
{
	RoomPieceCombinerInfo * copy = new RoomPieceCombinerInfo();
	*copy = *this;
	return RoomGeneratorInfoPtr(copy);
}

RoomPieceCombinerInfo::Set const * RoomPieceCombinerInfo::choose_set(Random::Generator & _generator) const
{
	if (sets.is_empty())
	{
		return nullptr;
	}
	return &sets[_generator.get_int(sets.get_size())];
}

RoomPart const * RoomPieceCombinerInfo::get_door_override(Name const & _outerName) const
{
	for_every(doorOverride, doorOverrides)
	{
		if (doorOverride->outerName == _outerName)
		{
			return doorOverride->roomPart.get();
		}
	}
	return nullptr;
}

PieceCombiner::Connector<RoomPieceCombinerGenerator> const * RoomPieceCombinerInfo::find_outer_connector(Name const & _outerName) const
{
	for_every_ref(outerRoomConnector, outerRoomConnectors)
	{
		if (outerRoomConnector->outerName == _outerName)
		{
			return outerRoomConnector;
		}
	}

	return nullptr;
}

bool RoomPieceCombinerInfo::generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	auto randomGeneratorForGeneration = _room->get_individual_random_generator();

	// setup generator
	PieceCombiner::Generator<RoomPieceCombinerGenerator> generator;
	RoomPieceCombinerGenerator::setup_generator(generator, this, _room->get_room_type()->get_library());

	// include doors
	for_every_ptr(door, _room->get_all_doors())
	{
		// invisible doors are fine here
		RoomPart const * roomPart = get_door_override(door->get_outer_name());
		if (!roomPart)
		{
			// use default if no outer available
			roomPart = door->get_door()->get_door_type()->get_room_part();
		}
		if (roomPart == nullptr)
		{
			error(TXT("no roomPart defined for door %S"), door->get_door()->get_door_type()->get_name().to_string().to_char());
			return false;
		}
		generator.add_piece(roomPart->get_as_room_piece(), RoomPieceCombinerGenerator::PieceInstanceData(door));

		// secondary part
		if (auto * sdt = door->get_door()->get_secondary_door_type())
		{
			RoomPart const * roomPart = sdt->get_room_part();
			generator.add_piece(roomPart->get_as_room_piece(), RoomPieceCombinerGenerator::PieceInstanceData(door));
		}
	}

	// add obligatory sets
	for_every(roomTypeSet, obligatorySets)
	{
		for_every(includeInstanceRoomPart, roomTypeSet->includeInstanceRoomParts)
		{
			generator.add_piece(includeInstanceRoomPart->get()->get_as_room_piece(), RoomPieceCombinerGenerator::PieceInstanceData());
		}
	}

	// add set if present
	if (RoomPieceCombinerInfo::Set const * roomTypeSet = choose_set(randomGeneratorForGeneration))
	{
		for_every(includeInstanceRoomPart, roomTypeSet->includeInstanceRoomParts)
		{
			generator.add_piece(includeInstanceRoomPart->get()->get_as_room_piece(), RoomPieceCombinerGenerator::PieceInstanceData());
		}
	}

	// generate generator
	PieceCombiner::GenerationResult::Type generationResult = generator.generate(&randomGeneratorForGeneration);

#ifdef LOG_WORLD_GENERATION
	float generationTime = startedGenerationTimeStamp.get_time_since();
	output(TXT("generated in %.3fs"), generationTime);
#endif

	if (generationResult != PieceCombiner::GenerationResult::Success)
	{
		return false;
	}

	if (!process_successful_generator(_room, REF_ generator))
	{
		return false;
	}

	if (!_subGenerator)
	{
		_room->mark_mesh_generated();
	}

	return true;
}


bool RoomPieceCombinerInfo::generate_sub_for(Room* _room, GeneratedRoomPart* _generatedRoomPart, int _randomOffset) const
{
	auto randomGeneratorForGeneration = _room->get_individual_random_generator();
	_randomOffset += 1;
	randomGeneratorForGeneration.advance_seed(_randomOffset, _randomOffset * 5);

	an_assert(_generatedRoomPart->roomPartInstance->get_room_part(), TXT("if this part instance doesn't have room part, why are we here?"));
	an_assert(_generatedRoomPart->roomPartInstance->get_room_part()->get_sub_room_piece_combiner_generator_info());
	RoomPart const * roomPart = _generatedRoomPart->roomPartInstance->get_room_part();
#ifdef LOG_WORLD_GENERATION
	::System::TimeStamp startedGenerationTimeStamp;
	startedGenerationTimeStamp.reset();
	output(TXT("generating sub room %S..."), roomPart->get_name().to_string().to_char());
#endif

	RoomPieceCombinerInfo const* subRoomPieceCombinerInfo = roomPart->get_sub_room_piece_combiner_generator_info();

	// setup generator
	PieceCombiner::Generator<RoomPieceCombinerGenerator> generator;
	RoomPieceCombinerGenerator::setup_generator(generator, subRoomPieceCombinerInfo, roomPart->get_library());

	PieceCombiner::PieceInstance<RoomPieceCombinerGenerator>* piece = _generatedRoomPart->piece;

	bool anythingAdded = false;

	// include connectors of [this piece]=[pieceVoid] as outer connectors for [generator]
	an_assert(!piece->get_owned_connectors().is_empty());
	// add outer connectors
	for_every_ptr(ownedConnector, piece->get_owned_connectors())
	{
		PieceCombiner::Connector<RoomPieceCombinerGenerator> const * connectorType = ownedConnector->get_connector();
		if (connectorType->name.is_valid())
		{
			// check if for this connector there is "outer" inside this region type
			if (PieceCombiner::Connector<RoomPieceCombinerGenerator> const * insideOuterConnectorType = subRoomPieceCombinerInfo->find_outer_connector(connectorType->name))
			{
				// add outer connector with placement
				RoomPieceCombinerGenerator::ConnectorInstanceData insideOuterConnectorData = ownedConnector->get_data();
				// calculate local (to piece) connector placement
				Transform localConnectorPlacement;
				RoomPieceCombinerGenerator::ConnectorDef::get_placement(ownedConnector->get_connector()->def, OUT_ localConnectorPlacement);

				// calculate world placement - where it should be
				Transform const worldConnectorPlacement = piece->get_data().get_placement().to_world(localConnectorPlacement);

				// outer connector should have placement!
				insideOuterConnectorData.place_at(worldConnectorPlacement);

				generator.add_outer_connector(insideOuterConnectorType, ownedConnector, insideOuterConnectorData, PieceCombiner::ConnectorSide::opposite(ownedConnector->get_side()));

				anythingAdded = true;
			}
		}
	}

	// add obligatory sets
	for_every(roomTypeSet, subRoomPieceCombinerInfo->obligatorySets)
	{
		for_every(includeInstanceRoomPart, roomTypeSet->includeInstanceRoomParts)
		{
			generator.add_piece(includeInstanceRoomPart->get()->get_as_room_piece(), RoomPieceCombinerGenerator::PieceInstanceData());
		}
	}

	// add set if present
	if (RoomPieceCombinerInfo::Set const * roomPartSet = subRoomPieceCombinerInfo->choose_set(randomGeneratorForGeneration))
	{
		for_every(includeInstanceRoomPart, roomPartSet->includeInstanceRoomParts)
		{
			generator.add_piece(includeInstanceRoomPart->get()->get_as_room_piece(), RoomPieceCombinerGenerator::PieceInstanceData());
			anythingAdded = true;
		}
	}

	if (!anythingAdded)
	{
		an_assert(anythingAdded, TXT("won't generate sub room part as there was nothing added for generation, nor connector, nor room part instance"));
		return false;
	}

	// generate generator
	PieceCombiner::GenerationResult::Type generationResult = generator.generate(&randomGeneratorForGeneration);

#ifdef LOG_WORLD_GENERATION
	float generationTime = startedGenerationTimeStamp.get_time_since();
	output(TXT("generated in %.3fs"), generationTime);
#endif

	if (generationResult != PieceCombiner::GenerationResult::Success)
	{
		// try fallback generation then
		an_assert(false);
#ifdef LOG_WORLD_GENERATION
		output(TXT("room sub generation failed"));
#endif
		return false;
	}

	return process_successful_generator(_room, REF_ generator);
}

bool RoomPieceCombinerInfo::process_successful_generator(Room* _room, REF_ PieceCombiner::Generator<RoomPieceCombinerGenerator> & _generator) const
{
	bool result = true;

	// make sure that all pieces are placed
	RoomPieceCombinerGenerator::place_pieces(_generator);

	// if at least one piece is not placed, use fallback generation
	for_every_ptr(piece, _generator.access_all_piece_instances())
	{
		if (!piece->get_data().is_placed())
		{
			error(TXT("piece not placed!"));
			an_assert(false);
			return false;
		}
	}

	ARRAY_STACK(GeneratedRoomPart, roomPartsRequiringFurtherGeneration, _generator.access_all_piece_instances().get_size());

	for_every_ptr(piece, _generator.access_all_piece_instances())
	{
		if (piece->get_data().is_placed())
		{
			if (RoomPart * roomPart = piece->get_piece()->def.roomPart.get())
			{
				// add room part to room at placement of this piece
				RoomPartInstance* roomPartInstance = _room->add_room_part(roomPart, piece->get_data().get_placement());
				if (roomPart->get_sub_room_piece_combiner_generator_info())
				{
					roomPartsRequiringFurtherGeneration.push_back(GeneratedRoomPart(piece, roomPartInstance));
				}
			}
			if (piece->get_data().doorInRoom)
			{
				// place door in room
				piece->get_data().doorInRoom->set_placement(piece->get_data().get_placement());
			}
		}
		else
		{
			an_assert(false, TXT("piece not placed!"));
		}
	}

	for_every(roomPartRequiringFurtherGeneration, roomPartsRequiringFurtherGeneration)
	{
		result &= generate_sub_for(_room, roomPartRequiringFurtherGeneration, for_everys_index(roomPartRequiringFurtherGeneration));
	}

	return result;
}
