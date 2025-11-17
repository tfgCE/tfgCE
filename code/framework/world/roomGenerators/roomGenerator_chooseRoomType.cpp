#include "roomGenerator_chooseRoomType.h"

#include "..\room.h"

#include "..\..\library\library.h"

#include "..\..\..\core\random\randomUtils.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"

//

using namespace Framework;
using namespace RoomGenerators;

//

REGISTER_FOR_FAST_CAST(ChooseRoomType);

ChooseRoomType::ChooseRoomType()
{
}

ChooseRoomType::~ChooseRoomType()
{
}

bool ChooseRoomType::load_entries(Array<Entry> & _entries, IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("choose")))
	{
		Entry e;
		e.probabilityCoef = node->get_float_attribute_or_from_child(TXT("probCoef"), e.probabilityCoef);
		e.probabilityCoef = node->get_float_attribute_or_from_child(TXT("probabilityCoef"), e.probabilityCoef);
		if (e.roomType.load_from_xml(node, TXT("roomType")) &&
			e.roomType.is_set())
		{
			_lc.load_group_into(e.roomType);
			_entries.push_back(e);
		}
		else
		{
			error_loading_xml(node, TXT("room type not provided"));
			result = false;
		}
	}

	return result;
}

bool ChooseRoomType::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= load_entries(entries, _node, _lc);

	return result;
}

bool ChooseRoomType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	return base::prepare_for_game(_library, _pfgContext);
}

bool ChooseRoomType::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	todo_important(TXT("implement_!"));
	return true;
}

RoomGeneratorInfoPtr ChooseRoomType::create_copy() const
{
	todo_important(TXT("implement_!"));
	RoomGeneratorInfoPtr copy;
	return copy;
}


bool ChooseRoomType::generate(Framework::Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	result &= generate_from_entries(entries, _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::generate(_room, _subGenerator, REF_ _roomGeneratingContext);

	return result;
}

bool ChooseRoomType::generate_from_entries(Array<Entry> const & _entries, Framework::Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext)
{
	scoped_call_stack_info(TXT("ChooseRoomType::generate_from_entries"));

	Random::Generator rg = _room->get_individual_random_generator();
	rg.advance_seed(2349790, 9349075);

	bool result = true;
	int idx = RandomUtils::ChooseFromContainer<Array<Entry>, Entry>::choose(rg, _entries, [](Entry const & _entry) { return _entry.probabilityCoef; });
	if (_entries.is_index_valid(idx))
	{
		auto const & entry = _entries[idx];
		auto entryRoomType = entry.roomType;
		entryRoomType.fill_value_with(_room->get_variables());
		auto* roomType = Library::get_current()->get_room_types().find(entryRoomType.get());
		if (roomType)
		{
			_room->set_room_type(roomType);
			result = _room->generate_room_using_room_generator(REF_ _roomGeneratingContext);
		}
		else
		{
			result = false;
		}
	}

	return result;

}
