#include "roomGenerator_chooseRoomTypeForCorridor.h"

#include "..\doorInRoom.h"
#include "..\room.h"
#include "..\world.h"

#include "..\..\library\library.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\meshGen\meshGenValueDefImpl.inl"

#include "..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace RoomGenerators;

//

#define VAR(type, name) name.load_from_xml(_node, TXT(#name))

bool ChooseRoomTypeForCorridor::Entry::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;
	result &= roomType.load_from_xml(_node, TXT("roomType"));
	result &= worldSettingCondition.load_from_xml(_node, TXT("worldSettingCondition"));
	_lc.load_group_into(roomType);
	error_loading_xml_on_assert(roomType.is_set() || worldSettingCondition.is_set(), result, _node, TXT("no room type provided"));
	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probCoef"), probabilityCoef);
	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probabilityCoef"), probabilityCoef);
	probabilityCoefParam = _node->get_name_attribute_or_from_child(TXT("probCoefParam"), probabilityCoefParam);
	probabilityCoefParam = _node->get_name_attribute_or_from_child(TXT("probabilityCoefParam"), probabilityCoefParam);
	disallowIfNeighboursWithTagged.load_from_xml_attribute_or_child_node(_node, TXT("disallowIfNeighboursWithTagged"));
	#include "roomGenerator_chooseRoomTypeForCorridor_list.h"
	return result;
}

#undef VAR

//

REGISTER_FOR_FAST_CAST(ChooseRoomTypeForCorridor);

ChooseRoomTypeForCorridor::ChooseRoomTypeForCorridor()
{
}

ChooseRoomTypeForCorridor::~ChooseRoomTypeForCorridor()
{
}

bool ChooseRoomTypeForCorridor::load_entries(Array<Entry> & _entries, IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("choose")))
	{
		Entry e;
		if (e.load_from_xml(node, _lc))
		{
			_entries.push_back(e);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool ChooseRoomTypeForCorridor::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= load_entries(entries, _node, _lc);

	return result;
}

bool ChooseRoomTypeForCorridor::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	return base::prepare_for_game(_library, _pfgContext);
}

bool ChooseRoomTypeForCorridor::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	todo_important(TXT("implement_!"));
	return true;
}

RoomGeneratorInfoPtr ChooseRoomTypeForCorridor::create_copy() const
{
	todo_important(TXT("implement_!"));
	RoomGeneratorInfoPtr copy;
	return copy;
}


bool ChooseRoomTypeForCorridor::generate(Framework::Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	result &= generate_from_entries(entries, _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::generate(_room, _subGenerator, REF_ _roomGeneratingContext);

	return result;
}

#define VAR(type, var) \
	if (entry->var.is_set() && \
		!entry->var.get().is_empty() && \
		! entry->var.get().does_contain(var)) \
	{ \
		continue; \
	}

bool ChooseRoomTypeForCorridor::generate_from_entries(Array<Entry> const & _entries, Framework::Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext)
{
	scoped_call_stack_info(TXT("ChooseRoomTypeForCorridor::generate_from_entries"));

	if (_room->get_all_doors().get_size() != 2)
	{
		error(TXT("corridor should have 2 doors"));
		return false;
	}

	Random::Generator rg = _room->get_individual_random_generator();
	rg.advance_seed(2349790, 9349075);

	Framework::DoorInRoom * dA = _room->get_all_doors()[0];
	Framework::DoorInRoom * dB = _room->get_all_doors()[1];
	Transform vrPlacementA = dA->get_vr_space_placement();
	Transform vrPlacementB = dB->get_vr_space_placement();

	Transform doorA = turn_matrix_xy_180(vrPlacementA.to_matrix()).to_transform();
	Transform doorB = turn_matrix_xy_180(vrPlacementB.to_matrix()).to_transform();
	
	Vector3 relLoc = doorA.location_to_local(doorB.get_translation());

	float absAngleTowards = abs(Rotator3::normalise_axis(relLoc.to_rotator().yaw));

	float dot = Vector3::dot(doorA.get_axis(Axis::Y), doorB.get_axis(Axis::Y));
	float absAngle = acos_deg(clamp(dot, -1.0f, 1.0f));

	float x = relLoc.x;
	float y = relLoc.y;
	float absX = abs(x);
	float dist = relLoc.length_2d();

#ifdef LOG_WORLD_GENERATION
	output(TXT("choose for corridor from:"));
#endif

	ARRAY_STACK(Entry const *, entries, _entries.get_size());
	for_every(entry, _entries)
	{
		#include "roomGenerator_chooseRoomTypeForCorridor_list.h"
		if (!entry->disallowIfNeighboursWithTagged.is_empty())
		{
			bool isOk = true;
			for_every_ptr(door, _room->get_all_doors())
			{
				// invisible doors are fine here
				// world separator doors always allow anything
				if (!door->get_door()->is_world_separator_door())
				{
					if (auto* room = door->get_room_on_other_side())
					{
						if (entry->disallowIfNeighboursWithTagged.check(room->get_tags()))
						{
							isOk = false;
							break;
						}
					}
				}
			}
			if (!isOk)
			{
				continue;
			}
		}
#ifdef LOG_WORLD_GENERATION
		if (entry->roomType.is_set())
		{
			auto entryRoomType = entry->roomType;
			entryRoomType.fill_value_with(_room->get_variables());
			auto* roomType = Library::get_current()->get_room_types().find(entryRoomType.get());
			if (roomType)
			{
				if (entry->probabilityCoefParam.is_valid())
				{
					output(TXT(" + (%.3f x %.3f[%S]) %S"), entry->probabilityCoef, _room->get_value<float>(entry->probabilityCoefParam, 1.0f), entry->probabilityCoefParam.to_char(), roomType->get_name().to_string().to_char());
				}
				else
				{
					output(TXT(" + (%.3f) %S"), entry->probabilityCoef, roomType->get_name().to_string().to_char());
				}
			}
		}
		if (entry->worldSettingCondition.is_set())
		{
			auto entryWorldSettingCondition = entry->worldSettingCondition;
			entryWorldSettingCondition.fill_value_with(_room->get_variables());
			Array<LibraryStored*> roomTypes;
			WorldSettingConditionContext wscc(_room);
			wscc.get(Library::get_current(), RoomType::library_type(), roomTypes, entryWorldSettingCondition.get());
			if (entry->probabilityCoefParam.is_valid())
			{
				output(TXT(" + (%.3f x %.3f[%S]) %S"), entry->probabilityCoef, _room->get_value<float>(entry->probabilityCoefParam, 1.0f), entry->probabilityCoefParam.to_char(), entryWorldSettingCondition.get().to_string().to_char());
			}
			else
			{
				output(TXT(" + (%.3f) %S"), entry->probabilityCoef, entryWorldSettingCondition.get().to_string().to_char());
			}
			if (roomTypes.is_empty())
			{
				output(TXT("   --!won't be used because is empty!--"));
			}
			else
			{
				for_every_ptr(rt, roomTypes)
				{
					output(TXT("   %S"), rt->get_name().to_string().to_char());
				}
			}
		}
#endif
		entries.push_back(entry);
	}

	if (entries.is_empty())
	{
		error(TXT("no candidates to use!"));
		return false;
	}

	bool result = false;
	bool keepTrying = true;
	while (keepTrying && ! entries.is_empty())
	{
		int idx = RandomUtils::ChooseFromContainer<ArrayStack<Entry const *>, Entry const *>::choose(rg, entries, [_room](Entry const * const & _entry)
			{
				float probCoef = _entry->probabilityCoef;
				if (_entry->probabilityCoefParam.is_valid())
				{
					probCoef *= _room->get_value<float>(_entry->probabilityCoefParam, 1.0f);
				}
				return probCoef;
			});
		if (entries.is_index_valid(idx))
		{
			auto const & entry = *entries[idx];
			if (entry.roomType.is_set())
			{
				auto entryRoomType = entry.roomType;
				entryRoomType.fill_value_with(_room->get_variables());
				auto* roomType = Library::get_current()->get_room_types().find(entryRoomType.get());
				if (roomType)
				{
					scoped_call_stack_info(TXT("entry.roomType"));
#ifdef LOG_WORLD_GENERATION
					output(TXT("choose \"%S\" for corridor (through room type)"), roomType->get_name().to_string().to_char());
#endif
					_room->set_room_type(roomType);
					result = _room->generate_room_using_room_generator(REF_ _roomGeneratingContext);
					keepTrying = false;
				}
				else
				{
					entries.remove_fast_at(idx);
				}
			}
			else if (entry.worldSettingCondition.is_set())
			{
				auto entryWorldSettingCondition = entry.worldSettingCondition;
				entryWorldSettingCondition.fill_value_with(_room->get_variables());
				Array<LibraryStored*> roomTypes;
				WorldSettingConditionContext wscc(_room);
				wscc.get(Library::get_current(), RoomType::library_type(), roomTypes, entryWorldSettingCondition.get());
				if (! roomTypes.is_empty())
				{
					int roomTypeIdx = RandomUtils::ChooseFromContainer<Array<LibraryStored*>, LibraryStored*>::choose(rg, roomTypes, LibraryStored::get_prob_coef_from_tags<LibraryStored>);
					if (auto* roomType = fast_cast<RoomType>(roomTypes[roomTypeIdx]))
					{
						scoped_call_stack_info(TXT("entry.worldSettingCondition"));
#ifdef LOG_WORLD_GENERATION
						output(TXT("choose \"%S\" for corridor (through world setting condition)"), roomType->get_name().to_string().to_char());
#endif
						_room->set_room_type(roomType);
						result = _room->generate_room_using_room_generator(REF_ _roomGeneratingContext);
						keepTrying = false;
					}
					else
					{
						entries.remove_fast_at(idx);
					}
				}
				else
				{
					entries.remove_fast_at(idx);
				}
			}
			else
			{
				result = false;
			}
		}
	}

	return result;

}

#undef VAR