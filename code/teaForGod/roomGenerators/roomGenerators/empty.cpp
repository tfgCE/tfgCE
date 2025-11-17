#include "empty.h"

#include "..\roomGenerationInfo.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_VARIABLES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

REGISTER_FOR_FAST_CAST(Empty);

Empty::Empty()
{
}

Empty::~Empty()
{
}

bool Empty::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool Empty::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::RoomGeneratorInfoPtr Empty::create_copy() const
{
	Empty* copy = new Empty();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool Empty::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("empty (room) \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}

	_room->place_pending_doors_on_pois(); 
	_room->mark_vr_arranged();
	_room->mark_mesh_generated();

	return result;
}
