#include "roomGeneratorInfo.h"

#include "doorTypeReplacer.h"
#include "room.h"

#include "roomGenerators\roomGenerator_chooseRoomType.h"
#include "roomGenerators\roomGenerator_chooseRoomTypeForCorridor.h"
#include "roomGenerators\roomGenerator_roomPieceCombinerInfo.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\world\doorInRoom.h"

#include "..\..\core\other\factoryOfNamed.h"

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(RoomGeneratorInfo);

typedef FactoryOfNamed<RoomGeneratorInfo, String> RoomGeneratorInfoFactory;

void RoomGeneratorInfo::register_room_generator_info(String const & _name, RoomGeneratorInfo::Construct _func)
{
	RoomGeneratorInfoFactory::add(_name, _func);
}

void RoomGeneratorInfo::initialise_static()
{
	RoomGeneratorInfoFactory::initialise_static();

	register_room_generator_info(String(TXT("choose room type")), []() { return new RoomGenerators::ChooseRoomType(); });
	register_room_generator_info(String(TXT("choose room type for corridor")), []() { return new RoomGenerators::ChooseRoomTypeForCorridor(); });
	register_room_generator_info(String(TXT("piece combiner")), []() { return new RoomGenerators::RoomPieceCombinerInfo(); });
}

void RoomGeneratorInfo::close_static()
{
	RoomGeneratorInfoFactory::close_static();
}

RoomGeneratorInfo* RoomGeneratorInfo::create(String const & _name)
{
	auto* rgi = RoomGeneratorInfoFactory::create(_name);
#ifdef AN_DEVELOPMENT_OR_PROFILER
	rgi->name = _name;
#endif
	return rgi;
}

bool RoomGeneratorInfo::generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	if (placeDoorOnPOI.is_valid() && !_room->get_all_doors().is_empty())
	{
		an_assert(_room->get_all_doors().get_size() == 1, TXT("we now support only single door"));
		Random::Generator rg = _room->get_individual_random_generator();
		rg.advance_seed(23597, 97345);
		ARRAY_STACK(Framework::PointOfInterestInstance*, pois, 32);
		_room->for_every_point_of_interest(placeDoorOnPOI, [&pois, &rg](Framework::PointOfInterestInstance * _fpoi) {pois.push_back_or_replace(_fpoi, rg); });
		if (!pois.is_empty())
		{
			Framework::PointOfInterestInstance* poi = pois[rg.get_int(pois.get_size())];
			an_assert(poi);
			auto* dir = _room->get_all_doors().get_first();
			dir->set_placement(poi->calculate_placement());
		}
		else
		{
			error(TXT("no poi \"%S\" found to place door"), placeDoorOnPOI.to_char());
		}
	}

	return true;
}

bool RoomGeneratorInfo::post_generate(Room* _room, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	return true;
}

bool RoomGeneratorInfo::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	roomGeneratorPriority = _node->get_int_attribute_or_from_child(TXT("priority"), roomGeneratorPriority);
	roomGeneratorPriority = _node->get_int_attribute_or_from_child(TXT("roomGeneratorPriority"), roomGeneratorPriority);

	warn_loading_xml_on_assert(!_node->first_child_named(TXT("roomParameters")), _node, TXT("roomParameters deprecated, use parameters"));
	result &= generationParameters.load_from_xml_child_node(_node, TXT("parameters"));
	_lc.load_group_into(generationParameters);

	result &= doorTypeReplacer.load_from_xml(_node, TXT("doorTypeReplacer"));
	result &= _lc.load_group_into(doorTypeReplacer);

	placeDoorOnPOI = _node->get_name_attribute_or_from_child(TXT("placeDoorOnPOI"), placeDoorOnPOI);

	return result;
}

bool RoomGeneratorInfo::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

bool RoomGeneratorInfo::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	if (doorTypeReplacer.is_value_set())
	{
		result &= _renamer.apply_to(doorTypeReplacer.access());
	}

	return true;
}

bool RoomGeneratorInfo::apply_generation_parameters_to(Room* _room) const
{
	bool result = true;

	_room->access_variables().set_from(generationParameters);

	return result;
}

bool RoomGeneratorInfo::apply_generation_parameters_to(SimpleVariableStorage & _variables) const
{
	bool result = true;

	_variables.set_from(generationParameters);

	return result;
}

RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> RoomGeneratorInfo::generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region* _region, REF_ Random::Generator & _randomGenerator) const
{
	return RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>>();
}

DoorTypeReplacer * RoomGeneratorInfo::get_door_type_replacer(Room* _room) const
{
	if (doorTypeReplacer.is_set())
	{
		LibraryName dtrName;
		if (doorTypeReplacer.is_value_set())
		{
			dtrName = doorTypeReplacer.get_value();
		}
		else if (doorTypeReplacer.get_value_param().is_set())
		{
			Name const & varName = doorTypeReplacer.get_value_param().get();
			_room->restore_value(varName, dtrName);
		}
		if (dtrName.is_valid())
		{
			if (auto* dtr = Library::get_current()->get_door_type_replacers().find(dtrName))
			{
				return dtr;
			}
		}
	}
	return nullptr;
}
