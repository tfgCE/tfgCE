#include "doorVRCorridor.h"

#include "..\library\libraryPrepareContext.h"
#include "..\library\libraryStored.h"
#include "..\world\roomType.h"

#include "..\..\core\random\random.h"
#include "..\..\core\random\randomUtils.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

using namespace Framework;

//


bool DoorVRCorridor::Entry::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probCoef"), probabilityCoef);
	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probabilityCoef"), probabilityCoef);
	result &= roomType.load_from_xml(_node, TXT("roomType"), _lc);
	if (!roomType.is_name_valid())
	{
		error_loading_xml(_node, TXT("no roomType provided"));
		result = false;
	}

	return result;
}

bool DoorVRCorridor::Entry::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= roomType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

bool DoorVRCorridor::Entry::apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library)
{
	bool result = true;

	result &= _renamer.apply_to(roomType, _library);

	return result;
}

//

bool DoorVRCorridor::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("doorVRCorridorRoom")))
	{
		Entry entry;
		if (entry.load_from_xml(node, _lc))
		{
			rooms.push_back(entry);
		}
		else
		{
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("doorVRCorridorElevator")))
	{
		Entry entry;
		if (entry.load_from_xml(node, _lc))
		{
			elevators.push_back(entry);
		}
		else
		{
			result = false;
		}
	}
	priority.load_from_xml(_node->first_child_named(TXT("doorVRCorridor")), TXT("priority"));

	return result;
}

bool DoorVRCorridor::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(entry, rooms)
	{
		result &= entry->prepare_for_game(_library, _pfgContext);
	}
	for_every(entry, elevators)
	{
		result &= entry->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

RoomType* DoorVRCorridor::get_room(Random::Generator& _rg) const
{
	if (rooms.is_empty())
	{
		return nullptr;
	}

	int idx = RandomUtils::ChooseFromContainer<Array<Entry>, Entry>::choose(_rg, rooms,
		[](Entry const & _option)
	{
		return _option.probabilityCoef * LibraryStored::get_prob_coef_from_tags(_option.roomType.get());
	}
	);

	return rooms[idx].roomType.get();
}

RoomType* DoorVRCorridor::get_elevator(Random::Generator& _rg) const
{
	if (elevators.is_empty())
	{
		return nullptr;
	}

	int idx = RandomUtils::ChooseFromContainer<Array<Entry>, Entry>::choose(_rg, elevators,
		[](Entry const & _option)
	{
		return _option.probabilityCoef * LibraryStored::get_prob_coef_from_tags(_option.roomType.get());
	}
	);

	return elevators[idx].roomType.get();
}

bool DoorVRCorridor::get_priority(Random::Generator& _rg, OUT_ float & _result) const
{
	if (priority.is_set())
	{
		_result = _rg.get(priority.get());
		return true;
	}
	else
	{
		return false;
	}
}

bool DoorVRCorridor::get_priority_range(OUT_ Range & _result) const
{
	if (priority.is_set())
	{
		_result = priority.get();
		return true;
	}
	else
	{
		return false;
	}
}

DoorVRCorridor DoorVRCorridor::create_copy() const
{
	return *this; // should be enough
}

bool DoorVRCorridor::apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library)
{
	bool result = true;
	for_every(entry, rooms)
	{
		result &= entry->apply_renamer(_renamer, _library);
	}
	for_every(entry, elevators)
	{
		result &= entry->apply_renamer(_renamer, _library);
	}
	return result;
}

void DoorVRCorridor::clear_room_types()
{
	rooms.clear();
	elevators.clear();
}
