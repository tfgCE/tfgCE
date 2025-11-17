#include "roomPieceCombinerGenerator.h"

#include "roomGenerators\roomGenerator_roomPieceCombinerInfo.h"

#include "..\library\library.h"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

using namespace Framework;

//#define ROOM_GENERATOR_EXTREME_DETAILS

#ifdef ROOM_GENERATOR_EXTREME_DETAILS
#define log_generator(...) log_to_file(__VA_ARGS__)
#else
#define log_generator(...)
#endif

RoomPartConnector::Type RoomPartConnector::parse(String const & _string)
{
	if (_string == TXT("normal") ||
		_string == TXT("inPlace"))
	{
		return RoomPartConnector::Normal;
	}
	else if (_string == TXT("outward") ||
			 _string == TXT("bidirectional") ||
			 _string == TXT("bidirect"))
	{
		return RoomPartConnector::Outward;
	}
	error(TXT("room part connector type not recognised"));
	return RoomPartConnector::Normal;
}

//

bool RoomPieceCombinerGenerator::Hull::load_from_xml(IO::XML::Node const * _node, LoadingContext * _context)
{
	bool result = true;

	result &= hull.load_from_xml(_node);
	result &= blocks.load_from_xml(_node, String(TXT("blocks")));
	result &= blockedBy.load_from_xml(_node, String(TXT("blockedBy")));

	return result;
}

//

RoomPieceCombinerGenerator::PieceDef::PieceDef()
: canBeUsedForPlacementInRoom(true)
, initialPlacement(Transform::identity)
{
}

bool RoomPieceCombinerGenerator::PieceDef::load_from_xml(IO::XML::Node const * _node, LoadingContext * _context)
{
	bool result = true;

	canBeUsedForPlacementInRoom = _node->get_bool_attribute(TXT("canBeUsedForPlacementInRoom"), canBeUsedForPlacementInRoom);

	initialPlacement.load_from_xml_child_node(_node, TXT("initialPlacement"));

	result &= roomPart.load_from_xml(_node, TXT("roomPart"), _context->libraryLoadingContext);

	for_every(hullNode, _node->children_named(TXT("hull")))
	{
		Hull hull;
		if (hull.load_from_xml(hullNode))
		{
			hulls.push_back(hull);
		}
		else
		{
			error_loading_xml(hullNode, TXT("error while loading hull"));
			result = false;
		}
	}

	return result;
}

bool RoomPieceCombinerGenerator::PieceDef::prepare(PieceCombiner::Piece<RoomPieceCombinerGenerator> * _piece, PreparingContext * _context)
{
	bool result = true;

	result &= roomPart.find(_context->library);

	return result;
}

bool RoomPieceCombinerGenerator::PieceDef::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _renamer.apply_to(roomPart, _library);

	return result;
}

String RoomPieceCombinerGenerator::PieceDef::get_desc() const
{
	return roomPart.get() ? roomPart->get_name().to_string() : roomPart.get_name().to_string();
}

//

RoomPieceCombinerGenerator::ConnectorDef::ConnectorDef()
: type(RoomPartConnector::Normal)
, placement(Transform::identity)
{
}

bool RoomPieceCombinerGenerator::ConnectorDef::load_from_xml(IO::XML::Node const * _node, LoadingContext * _context)
{
	bool result = true;

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("type")))
	{
		type = RoomPartConnector::parse(attr->get_as_string());
	}
	
	socketName = _node->get_name_attribute(TXT("socket"), socketName);
	placement.load_from_xml_child_node(_node, TXT("placement"));

	return result;
}

bool RoomPieceCombinerGenerator::ConnectorDef::prepare(PieceCombiner::Piece<RoomPieceCombinerGenerator> * _piece, PieceCombiner::Connector<RoomPieceCombinerGenerator> * _connector, PreparingContext * _context)
{
	bool result = true;

	if (socketName.is_valid())
	{
		todo_important(TXT("look for sockets"));
	}
	
	return result;
}

bool RoomPieceCombinerGenerator::ConnectorDef::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	return result;
}

String RoomPieceCombinerGenerator::ConnectorDef::get_desc() const
{
	return socketName.to_string();
}

static void turn_around_z_axis(REF_ Transform & _t)
{
	Matrix44 tOrMat = _t.get_orientation().to_matrix();
	tOrMat.set_x_axis(-tOrMat.get_x_axis());
	tOrMat.set_y_axis(-tOrMat.get_y_axis());
}

void RoomPieceCombinerGenerator::ConnectorDef::get_placements(ConnectorDef const & _a, OUT_ Transform & _aPlacement, ConnectorDef const & _b, OUT_ Transform & _bPlacement)
{
	_aPlacement = _a.placement;
	_bPlacement = _b.placement;
	
	if (_a.type == RoomPartConnector::Outward &&
		_b.type == RoomPartConnector::Outward)
	{
		turn_around_z_axis(REF_ _bPlacement);
	}
	else if (_a.type == RoomPartConnector::Normal &&
			 _b.type == RoomPartConnector::Outward)
	{
		turn_around_z_axis(REF_ _bPlacement);
	}
	else if (_a.type == RoomPartConnector::Outward &&
			 _b.type == RoomPartConnector::Normal)
	{
		turn_around_z_axis(REF_ _aPlacement);
	}
}

void RoomPieceCombinerGenerator::ConnectorDef::get_placement(ConnectorDef const & _a, OUT_ Transform & _aPlacement)
{
	_aPlacement = _a.placement;

	if (_a.type == RoomPartConnector::Outward)
	{
		turn_around_z_axis(REF_ _aPlacement);
	}
}

//

RoomPieceCombinerGenerator::PieceInstanceData::PieceInstanceData()
: doorInRoom(nullptr)
, placementGroup(0)
{
}

RoomPieceCombinerGenerator::PieceInstanceData::PieceInstanceData(DoorInRoom* _dir)
: doorInRoom(_dir)
, placementGroup(0)
{
}

//

RoomPieceCombinerGenerator::ConnectorInstanceData::ConnectorInstanceData()
: hasProvidedPlacement(false)
{
}

void RoomPieceCombinerGenerator::ConnectorInstanceData::place_at(Transform const & _placement)
{
	hasProvidedPlacement = true;
	placement = _placement;
}

//

bool RoomPieceCombinerGenerator::Utils::update_and_validate(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator)
{
	if (!RoomPieceCombinerGenerator::place_pieces(_generator, true))
	{
		log_generator(TXT("pieces placement invalid"));
		return false;
	}
	if (! check_if_dont_intersect(_generator))
	{
		log_generator(TXT("intersect"));
		return false;
	}
	return true;
}

bool RoomPieceCombinerGenerator::Utils::check_if_dont_intersect(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator)
{
	for_every_ptr(piece, _generator.access_all_piece_instances())
	{
		if (piece->get_data().is_placed())
		{
			int placementGroup = piece->get_data().get_placement_group();
			for_every_ptr_continue_after(otherPiece, piece)
			{
				if (otherPiece->get_data().is_placed() &&
					otherPiece->get_data().get_placement_group() == placementGroup)
				{
					if (check_if_pieces_intersect(*piece, *otherPiece))
					{
#ifdef LOG_PIECE_COMBINER_FAILS
						log_generator(TXT("! %S (%S) and %S (%S) intersect"),
							piece->get_piece()->def.get_desc().to_char(), piece->get_data().get_placement().get_translation().to_string().to_char(),
							otherPiece->get_piece()->def.get_desc().to_char(), otherPiece->get_data().get_placement().get_translation().to_string().to_char());
#endif
						return false;
					}
				}
			}
		}
	}
	
	return true;
}

bool RoomPieceCombinerGenerator::Utils::check_if_pieces_intersect(PieceCombiner::PieceInstance<RoomPieceCombinerGenerator> const & _a, PieceCombiner::PieceInstance<RoomPieceCombinerGenerator> const & _b)
{
	PieceDef const & aDef = _a.get_piece()->def;
	PieceDef const & bDef = _b.get_piece()->def;
	for_every(aHull, aDef.hulls)
	{
		for_every(bHull, bDef.hulls)
		{
			// only if one blocks another (or vice versa)
			if (aHull->blockedBy.does_match_any_from(bHull->blocks) ||
				bHull->blockedBy.does_match_any_from(aHull->blocks))
			{
				// check if hulls actually intersect
				if (ConvexHull::does_intersect(aHull->hull, _a.get_data().get_placement(),
											   bHull->hull, _b.get_data().get_placement()))
				{
					return true;
				}
			}
		}
	}
	return false;
}

//
		
void RoomPieceCombinerGenerator::setup_generator(PieceCombiner::Generator<Framework::RoomPieceCombinerGenerator>& _generator, RoomGenerators::RoomPieceCombinerInfo const * _roomPieceCombinerGeneratorInfo, Library* _library)
{
	an_assert(_roomPieceCombinerGeneratorInfo);

	_generator.use_generation_rules(_roomPieceCombinerGeneratorInfo->get_generation_rules());
	_generator.use_update_and_validate(Utils::update_and_validate);

	for_every_ref(regionPiece, _roomPieceCombinerGeneratorInfo->get_room_pieces())
	{
		_generator.add_piece_type(regionPiece);
	}

	for_every(roomPart, _roomPieceCombinerGeneratorInfo->get_included_room_parts())
	{
		_generator.add_piece_type(roomPart->get()->get_as_room_piece());
	}
	
	if (_library)
	{
		for_every(tagCondition, _roomPieceCombinerGeneratorInfo->get_included_room_part_tag_conditions())
		{
			for_every_ptr(roomPart, _library->get_room_parts().get_tagged(*tagCondition->get()))
			{
				_generator.add_piece_type(roomPart->get_as_room_piece());
			}
		}
	}
}

bool RoomPieceCombinerGenerator::place_pieces(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator, bool _forValidation)
{
	bool result = true;

	log_generator(TXT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));

	// first, make all pieces as not placed
	for_every_ptr(piece, _generator.access_all_piece_instances())
	{
		an_assert(piece->get_piece()->def.roomPart.get(), TXT("used piece does not have room part associated"));
		piece->access_data().clear_placement();
	}
	
	int placementGroup = 0;

	// place through connectors (if they have placement provided)
	for_every_ptr(connector, _generator.access_all_connector_instances())
	{
		if (connector->get_data().has_provided_placement())
		{
			log_generator(TXT("start with connector %S/%S %S"),
				connector->get_connector()->def.get_desc().to_char(),
				connector->get_owner() ? connector->get_owner()->get_piece()->def.get_desc().to_char() : TXT(""),
				connector->get_data().get_placement().get_translation().to_string().to_char());
			if (auto toPiece = connector->get_owner_on_other_end())
			{
				// but place only those that are not placed!
				if (!toPiece->get_data().is_placed() &&
					((_forValidation) ||
					(!_forValidation && toPiece->get_piece()->def.canBeUsedForPlacementInRoom)))
				{
					++placementGroup;

					Transform localConnectorPlacement;
					RoomPieceCombinerGenerator::ConnectorDef::get_placement(connector->get_connector()->def, OUT_ localConnectorPlacement);
					Transform const & worldConnectorPlacement = connector->get_data().get_placement();

					// calculate where piece should be
					Transform piecePlacement = worldConnectorPlacement.to_world(localConnectorPlacement.inverted());

					toPiece->access_data().place_at(piecePlacement, placementGroup);
					if (!process_placement_for(_generator, toPiece, _forValidation))
					{
						log_generator(TXT("place through connectors (if they have placement provided)"));
						return false;
					}
				}
			}
		}
	}
	
#ifdef ROOM_GENERATOR_EXTREME_DETAILS
	int placedCountDebug = 0;
	for_every_ptr(piece, _generator.access_all_piece_instances())
	{
		// don't start with something that can't be used for placement in room (for example, door in room)
		if (piece->get_data().is_placed())
		{
			++ placedCountDebug;
		}
	}
	log_generator(TXT("placedCountDebug %i"), placedCountDebug);
#endif

	// place not placed in different placement groups
	for_every_ptr(piece, _generator.access_all_piece_instances())
	{
		// don't start with something that can't be used for placement in room (for example, door in room)
		if (!piece->get_data().is_placed() &&
			((_forValidation) ||
			 (!_forValidation && piece->get_piece()->def.canBeUsedForPlacementInRoom)))
		{
			log_generator(TXT("can be used for placement and was not placed - go!"));
			++placementGroup;
			piece->access_data().place_at(piece->get_piece()->def.initialPlacement, placementGroup);
			log_generator(TXT(">>>>>>>>>>>>>>>>>>>>"));
			if (!process_placement_for(_generator, piece, _forValidation))
			{
				log_generator(TXT("place not placed in different placement groups"));
				return false;
			}
			log_generator(TXT("<<<<<<<<<<<<<<<<<<<<"));
		}
	}
	log_generator(TXT("...................................."));

	return result;
}

#ifdef ROOM_GENERATOR_EXTREME_DETAILS
int get_index_of(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator, PieceCombiner::PieceInstance<RoomPieceCombinerGenerator>* _piece)
{
	for_every_ptr(piece, _generator.access_all_piece_instances())
	{
		if (piece == _piece)
		{
			return for_everys_index(piece);
		}
	}
	return NONE;
}
#endif

bool RoomPieceCombinerGenerator::process_placement_for(PieceCombiner::Generator<RoomPieceCombinerGenerator>& _generator, PieceCombiner::PieceInstance<RoomPieceCombinerGenerator>* _piece, bool _forValidation)
{
	bool result = true;
	
	int placementGroup = _piece->get_data().get_placement_group();

	for_every_ptr(connector, _piece->get_owned_connectors())
	{
		if (PieceCombiner::ConnectorInstance<RoomPieceCombinerGenerator>* otherConnector = connector->get_connector_on_other_end())
		{
			// we use local/world naming but world in fact is "room's space"
			// get local placements
			Transform localConnectorPlacement;
			Transform localOtherConnectorPlacement;
			ConnectorDef::get_placements(connector->get_connector()->def, OUT_ localConnectorPlacement, otherConnector->get_connector()->def, OUT_ localOtherConnectorPlacement);

			log_generator(TXT("piece %i. %S connector %S"),
				get_index_of(_generator, _piece),
				_piece->get_piece()->def.get_desc().to_char(),
				connector->get_connector()->linkName.to_char());
			log_generator(TXT("other piece %i. %S connector %S"),
				get_index_of(_generator, otherConnector->get_owner()),
				otherConnector->get_owner() ? otherConnector->get_owner()->get_piece()->def.get_desc().to_char() : TXT(""),
				otherConnector->get_connector()->linkName.to_char());
			log_generator(TXT("checking piece location:%S  rotation:%S"),
				_piece->get_data().get_placement().get_translation().to_string().to_char(),
				_piece->get_data().get_placement().get_orientation().to_rotator().to_string().to_char());
			log_generator(TXT("    local connector location:%S  rotation:%S"),
				localConnectorPlacement.get_translation().to_string().to_char(),
				localConnectorPlacement.get_orientation().to_rotator().to_string().to_char());
			log_generator(TXT("    local other connector location:%S  rotation:%S"),
				localOtherConnectorPlacement.get_translation().to_string().to_char(),
				localOtherConnectorPlacement.get_orientation().to_rotator().to_string().to_char());
			log_generator(TXT("    local other connector inverted location:%S  rotation:%S"),
				localOtherConnectorPlacement.inverted().get_translation().to_string().to_char(),
				localOtherConnectorPlacement.inverted().get_orientation().to_rotator().to_string().to_char());

			// calculate other's world placement - where it should be
			Transform const worldConnectorPlacement = _piece->get_data().get_placement().to_world(localConnectorPlacement);
			Transform const worldOtherPlacement = worldConnectorPlacement.to_world(localOtherConnectorPlacement.inverted());

			log_generator(TXT("    world connector location:%S  rotation:%S"),
				worldConnectorPlacement.get_translation().to_string().to_char(),
				worldConnectorPlacement.get_orientation().to_rotator().to_string().to_char());
			log_generator(TXT("    world other piece location:%S  rotation:%S"),
				worldOtherPlacement.get_translation().to_string().to_char(),
				worldOtherPlacement.get_orientation().to_rotator().to_string().to_char());

			if (otherConnector->get_data().has_provided_placement())
			{
				log_generator(TXT("check connector %S/%S %S"),
					connector->get_connector()->def.get_desc().to_char(),
					connector->get_owner() ? connector->get_owner()->get_piece()->def.get_desc().to_char() : TXT(""),
					connector->get_data().get_placement().get_translation().to_string().to_char());
				log_generator(TXT("against connector %S/%S %S"),
					otherConnector->get_connector()->def.get_desc().to_char(),
					otherConnector->get_owner() ? otherConnector->get_owner()->get_piece()->def.get_desc().to_char() : TXT(""),
					otherConnector->get_data().get_placement().get_translation().to_string().to_char());
					// localOtherConnectorPlacement is not relevant as we don't have piece
				// check if placements are corresponding
				// compare actual placement with what we expect it to be
				Vector3 locDiff = worldConnectorPlacement.get_translation() - otherConnector->get_data().get_placement().get_translation();
				if (locDiff.length_squared() > sqr(0.05f))
				{
					// doesn't match location
					
					log_generator(TXT("doesn't match location %.3f (%S v %S) (%S/%S v %S/%S)"), locDiff.length(),
						worldConnectorPlacement.get_translation().to_string().to_char(),
						otherConnector->get_data().get_placement().get_translation().to_string().to_char(),
						connector->get_connector()->def.get_desc().to_char(),
						connector->get_connector()->linkName.to_char(),
						otherConnector->get_connector()->def.get_desc().to_char(),
						otherConnector->get_connector()->linkName.to_char());
					return false;
				}
				Quat oriDiff = worldConnectorPlacement.get_orientation() - otherConnector->get_data().get_placement().get_orientation();
				if (oriDiff.length_squared() > sqr(0.05f))
				{
					// doesn't match orientation
					log_generator(TXT("doesn't match orientation %.3f (%S/%S v %S/%S)"), oriDiff.length(),
						connector->get_connector()->def.get_desc().to_char(),
						connector->get_connector()->linkName.to_char(),
						otherConnector->get_connector()->def.get_desc().to_char(),
						otherConnector->get_connector()->linkName.to_char());
					return false;
				}
				float scaDiff = worldConnectorPlacement.get_scale() - otherConnector->get_data().get_placement().get_scale();
				if (length_of(scaDiff) > 0.01f)
				{
					// doesn't match scale
					log_generator(TXT("doesn't match scale %.3f (%S/%S v %S/%S)"), length_of(scaDiff),
						connector->get_connector()->def.get_desc().to_char(),
						connector->get_connector()->linkName.to_char(),
						otherConnector->get_connector()->def.get_desc().to_char(),
						otherConnector->get_connector()->linkName.to_char());
					return false;
				}
			}

			if (PieceCombiner::PieceInstance<RoomPieceCombinerGenerator>* otherPiece = connector->get_owner_on_other_end())
			{				
				if (! otherPiece->get_data().is_placed())
				{
					// place at calculated world placement
					log_generator(TXT("through connector %S/%S %S"),
						connector->get_connector()->def.get_desc().to_char(),
						connector->get_owner() ? connector->get_owner()->get_piece()->def.get_desc().to_char() : TXT(""),
						worldConnectorPlacement.get_translation().to_string().to_char());
					log_generator(TXT("place %i. %S at location:%S rotation:%S"),
						get_index_of(_generator, otherPiece),
						otherPiece->get_piece()->def.get_desc().to_char(),
						worldOtherPlacement.get_translation().to_string().to_char(),
						worldOtherPlacement.get_orientation().to_rotator().to_string().to_char());
					otherPiece->access_data().place_at(worldOtherPlacement, placementGroup);
					result &= process_placement_for(_generator, otherPiece, _forValidation);
				}
				else if (otherPiece->get_data().get_placement_group() == placementGroup)
				{
					// compare actual placement with what we expect it to be
					Vector3 locDiff = worldOtherPlacement.get_translation() - otherPiece->get_data().get_placement().get_translation();
					if (locDiff.length_squared() > sqr(0.05f))
					{
						// doesn't match location
						log_generator(TXT("doesn't match location %.3f (%S v %S) (%S/%S v piece %S)"), locDiff.length(),
							worldOtherPlacement.get_translation().to_string().to_char(),
							otherPiece->get_data().get_placement().get_translation().to_string().to_char(),
							connector->get_connector()->def.get_desc().to_char(),
							connector->get_connector()->linkName.to_char(),
							otherPiece->get_piece()->def.get_desc().to_char());
						return false;
					}
					Quat oriDiff = worldOtherPlacement.get_orientation() - otherPiece->get_data().get_placement().get_orientation();
					if (oriDiff.length_squared() > sqr(0.05f))
					{
						// doesn't match orientation
						log_generator(TXT("doesn't match orientation %.3f (%S/%S v piece %S)"), oriDiff.length(),
							connector->get_connector()->def.get_desc().to_char(),
							connector->get_connector()->linkName.to_char(),
							otherPiece->get_piece()->def.get_desc().to_char());
						return false;
					}
					float scaDiff = worldOtherPlacement.get_scale() - otherPiece->get_data().get_placement().get_scale();
					if (length_of(scaDiff) > 0.01f)
					{
						// doesn't match scale
						log_generator(TXT("doesn't match scale %.3f (%S/%S v piece %S)"), length_of(scaDiff),
							connector->get_connector()->def.get_desc().to_char(),
							connector->get_connector()->linkName.to_char(),
							otherPiece->get_piece()->def.get_desc().to_char());
						return false;
					}
				}
				else if (!_forValidation)
				{
#ifdef AN_ASSERT
					log_generator(TXT("pieces belong to different group %S -> %i  v  %S -> %i"), _piece->get_piece()->def.get_desc().to_char(), placementGroup,
						otherPiece->get_piece()->def.get_desc().to_char(), otherPiece->get_data().get_placement_group());
#endif
					an_assert(false, TXT("this should not happen, as this means, two groups were not connected when they should be"));
					return false;
				}
			}
		}
	}

	return result;
}

bool RoomPieceCombinerGenerator::apply_renamer_to(PieceCombiner::Piece<RoomPieceCombinerGenerator> & _piece, LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _piece.def.apply_renamer(_renamer, _library);
	for_every(connector, _piece.connectors)
	{
		result &= apply_renamer_to(*connector, _renamer, _library);
	}

	return result;
}

bool RoomPieceCombinerGenerator::apply_renamer_to(PieceCombiner::Connector<RoomPieceCombinerGenerator> & _connector, LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _connector.def.apply_renamer(_renamer, _library);

	return result;
}
