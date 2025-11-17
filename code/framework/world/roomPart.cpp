#include "roomPart.h"

#include "room.h"
#include "roomGenerators\roomGenerator_roomPieceCombinerInfo.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#ifdef AN_CLANG
#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#endif

using namespace Framework;

//

DEFINE_STATIC_NAME(meshStatic);

//

RoomPart::Element::Element()
: placement(Transform::identity)
{
}

bool RoomPart::Element::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	result &= mesh.load_from_xml(_node, TXT("static"), _lc);
	placement.load_from_xml(_node);

	if (IO::XML::Node const * poisNode = _node->first_child_named(TXT("pois")))
	{
		for_every(poiNode, poisNode->children_named(TXT("poi")))
		{
			PointOfInterest* poi = new PointOfInterest();
			poi->add_ref();
			if (poi->load_from_xml(poiNode, _lc))
			{
				pois.push_back(PointOfInterestPtr(poi));
			}
			else
			{
				result = false;
				poi->release_ref();
			}
		}
	}

	return result;
}

bool RoomPart::Element::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	if (!mesh.prepare_for_game_find_may_fail(_library, _pfgContext, LibraryPrepareLevel::Resolve, NAME(meshStatic)))
	{
		result &= mesh.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	return result;
}

//

REGISTER_FOR_FAST_CAST(RoomPart);
LIBRARY_STORED_DEFINE_TYPE(RoomPart, roomPart);

RoomPart::RoomPart(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, roomPiece( new PieceCombiner::Piece<RoomPieceCombinerGenerator>() )
, subRoomPieceCombinerGeneratorInfo(nullptr)
{
	roomPiece->def.roomPart.lock(this);
}

void RoomPart::prepare_to_unload()
{
	base::prepare_to_unload();
	roomPiece->def.roomPart.unlock_and_clear();
}

RoomPart::~RoomPart()
{
	delete_and_clear(subRoomPieceCombinerGeneratorInfo);
}

bool RoomPart::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	for_every(meshNode, _node->children_named(TXT("mesh")))
	{
		Element element;
		if (element.load_from_xml(meshNode, _lc))
		{
			elements.push_back(element);
		}
		else
		{
			result = false;
		}
	}

	{	// room generator
		RoomPieceCombinerGenerator::LoadingContext loadingContext(_lc);

		result &= roomPiece->load_from_xml(_node->first_child_named(TXT("asRoomPiece")), &loadingContext);
	}

	if (IO::XML::Node const * poisNode = _node->first_child_named(TXT("pois")))
	{
		for_every(poiNode, poisNode->children_named(TXT("poi")))
		{
			PointOfInterest* poi = new PointOfInterest();
			poi->add_ref();
			if (poi->load_from_xml(poiNode, _lc))
			{
				pois.push_back(PointOfInterestPtr(poi));
			}
			else
			{
				result = false;
			}
			poi->release_ref(); // if its stored in pois array, it will be handled by refcountobj pointer
		}
	}

	todo_important(TXT("load different types"));
	if (IO::XML::Node const * asSubRoomPieceCombinerGeneratorNode = _node->first_child_named(TXT("asRoomSubGenerator")))
	{
		warn_loading_xml(_node, TXT("use \"asSubRoomPieceCombinerGenerator\" instead of \"asRoomSubGenerator\""));

		if (!subRoomPieceCombinerGeneratorInfo)
		{
			subRoomPieceCombinerGeneratorInfo = new RoomGenerators::RoomPieceCombinerInfo();
		}
		result &= subRoomPieceCombinerGeneratorInfo->load_from_xml(asSubRoomPieceCombinerGeneratorNode, _lc);
	}
	if (IO::XML::Node const * asSubRoomPieceCombinerGeneratorNode = _node->first_child_named(TXT("asSubRoomPieceCombinerGenerator")))
	{
		if (!subRoomPieceCombinerGeneratorInfo)
		{
			subRoomPieceCombinerGeneratorInfo = new RoomGenerators::RoomPieceCombinerInfo();
		}
		result &= subRoomPieceCombinerGeneratorInfo->load_from_xml(asSubRoomPieceCombinerGeneratorNode, _lc);
	}

	return result;
}

bool RoomPart::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	result &= LibraryStored::prepare_for_game(_library, _pfgContext);
	for_every(element, elements)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	if (subRoomPieceCombinerGeneratorInfo)
	{
		result &= subRoomPieceCombinerGeneratorInfo->prepare_for_game(_library, _pfgContext);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::SetupRoomPieceCombiner)
	{	// room generator
		RoomPieceCombinerGenerator::PreparingContext preparingContext(_library);

		result &= roomPiece->prepare(&preparingContext);
	}
	
	return result;
}
