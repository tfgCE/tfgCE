#include "doorBasedWalls.h"

#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

DEFINE_STATIC_NAME(elevatorWidth);
DEFINE_STATIC_NAME(elevatorEdgeTop);

DEFINE_STATIC_NAME(wallEdgeTop);
DEFINE_STATIC_NAME(wallHoleWidth);
DEFINE_STATIC_NAME(wallHoleLower);
DEFINE_STATIC_NAME(wallHoleUpper);

//

REGISTER_FOR_FAST_CAST(DoorBasedWallsInfo);

DoorBasedWallsInfo::DoorBasedWallsInfo()
{
}

DoorBasedWallsInfo::~DoorBasedWallsInfo()
{
}

bool DoorBasedWallsInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= wallSceneryType.load_from_xml(_node, TXT("wallSceneryType"));
	_lc.load_group_into(wallSceneryType);

	orderDir.load_from_xml(_node, TXT("orderDir"));
	unlimitedOrdered = _node->get_bool_attribute_or_from_child_presence(TXT("unlimitedOrdered"), unlimitedOrdered);
	for_every(node, _node->children_named(TXT("ordered")))
	{
		Framework::MeshGenParam<Framework::LibraryName> orderedWallSceneryType;
		if (orderedWallSceneryType.load_from_xml(node, TXT("wallSceneryType")))
		{
			_lc.load_group_into(orderedWallSceneryType);
			orderedWallSceneryTypes.push_back(orderedWallSceneryType);
		}
		else
		{
			result = false;
		}
	}

	wallHeightAbove.load_from_xml(_node, TXT("wallHeightAbove"));
	wallBigTopOffsetChance = _node->get_float_attribute_or_from_child(TXT("wallBigTopOffsetChance"), wallBigTopOffsetChance);
	wallsOfSameHeight = _node->get_bool_attribute_or_from_child_presence(TXT("wallsOfSameHeight"), wallsOfSameHeight);
	eachDoorUnique = _node->get_bool_attribute_or_from_child_presence(TXT("eachDoorUnique"), eachDoorUnique);
	wallOffsetLocation.load_from_xml_child_node(_node, TXT("wallOffsetLocation"));

	return result;
}

bool DoorBasedWallsInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool DoorBasedWallsInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr DoorBasedWallsInfo::create_copy() const
{
	DoorBasedWallsInfo * copy = new DoorBasedWallsInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool DoorBasedWallsInfo::generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	DoorBasedWalls sr(this);
	result &= sr.generate(Framework::Game::get(), _room, REF_ _roomGeneratingContext);

	result &= base::generate(_room, _subGenerator, REF_ _roomGeneratingContext);

	if (!_subGenerator)
	{
		_room->mark_mesh_generated();
	}

	return result;
}

//

DoorBasedWalls::DoorBasedWalls(DoorBasedWallsInfo const * _info)
: info(_info)
{
}

DoorBasedWalls::~DoorBasedWalls()
{
}

bool DoorBasedWalls::generate(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;

	Random::Generator rg = _room->get_individual_random_generator();
	rg.advance_seed(6895, 34597);

	SimpleVariableStorage variables;
	_room->collect_variables(variables);
	info->apply_generation_parameters_to(variables);

	Framework::SceneryType * wallSceneryType = nullptr;
	if (info->wallSceneryType.is_set())
	{
		wallSceneryType = Framework::Library::get_current()->get_scenery_types().find(info->wallSceneryType.get(variables));
	}
	ARRAY_PREALLOC_SIZE(Framework::SceneryType *, orderedWallSceneryTypes, info->orderedWallSceneryTypes.get_size());
	for_every(wstName, info->orderedWallSceneryTypes)
	{
		if (auto* wst = Framework::Library::get_current()->get_scenery_types().find(wstName->get(variables)))
		{
			orderedWallSceneryTypes.push_back(wst);
		}
	}
	{
		struct OrderedWallPlacement
		{
			Transform placement;
			float ordered;

			OrderedWallPlacement() {}
			/* not explicit */ OrderedWallPlacement(Transform const & _placement, float _ordered = 0.0f) : placement(_placement), ordered(_ordered) {}
		};
		float topOffset = rg.get_chance(info->wallBigTopOffsetChance) ? 5000.0f : 0.0f;
		Array<OrderedWallPlacement> wallPlacements;
		for_every_ptr(door, _room->get_all_doors())
		{
			Transform wallPlacement = door->get_placement();
			if (!info->eachDoorUnique)
			{
				bool alreadyExists = false;
				for_every(existing, wallPlacements)
				{
					if (Vector3::dot(wallPlacement.get_axis(Axis::Y), existing->placement.get_axis(Axis::Y)) > 0.995f)
					{
						an_assert(abs(Plane(wallPlacement.get_axis(Axis::Y), wallPlacement.get_translation()).get_in_front(existing->placement.get_translation())) < 0.1f, TXT("coplanar doors only!"));
						float top = max(wallPlacement.get_translation().z, existing->placement.get_translation().z);
						Vector3 edge = existing->placement.get_translation();
						edge.z = top;
						existing->placement.set_translation(edge);
						alreadyExists = true;
					}
				}
				if (alreadyExists)
				{
					break;
				}
			}
			wallPlacements.push_back(wallPlacement);
		}

		float unifiedZ = wallPlacements.get_first().placement.get_translation().z;
		for_every(wallPlacement, wallPlacements)
		{
			unifiedZ = max(unifiedZ, wallPlacements.get_first().placement.get_translation().z);
		}

		Range wallHeightAbove = info->wallHeightAbove.get(variables);

		unifiedZ += rg.get(wallHeightAbove) + topOffset;

		// reorder
		{
			Vector3 orderDir = info->orderDir.get(variables);

			for_every(wp, wallPlacements)
			{
				wp->ordered = Vector3::dot(orderDir, wp->placement.get_translation());
			}
			sort(wallPlacements, [](const void* _a, const void* _b)
			{
				OrderedWallPlacement const & a = *(plain_cast<OrderedWallPlacement>(_a));
				OrderedWallPlacement const & b = *(plain_cast<OrderedWallPlacement>(_b));
				if (a.ordered < b.ordered) return A_BEFORE_B;
				if (a.ordered > b.ordered) return B_BEFORE_A;
				return A_AS_B;
			});
		}

		for_every(wallPlacement, wallPlacements)
		{
			Framework::Scenery* wallScenery = nullptr;
			if (!orderedWallSceneryTypes.is_empty())
			{
				if (info->unlimitedOrdered)
				{
					// this way we should use use last one if we don't have enough
					wallSceneryType = orderedWallSceneryTypes[clamp(for_everys_index(wallPlacement), 0, orderedWallSceneryTypes.get_size() - 1)];
				}
				else if (orderedWallSceneryTypes.is_index_valid(for_everys_index(wallPlacement)))
				{
					wallSceneryType = orderedWallSceneryTypes[for_everys_index(wallPlacement)];
				}
				else
				{
					// clear
					wallSceneryType = nullptr;
				}
			}
			if (!wallSceneryType)
			{
				continue; // just skip
			}
			wallSceneryType->load_on_demand_if_required();

			_game->perform_sync_world_job(TXT("spawn scenery"), [&wallScenery, wallSceneryType, _room]()
			{
				wallScenery = new Framework::Scenery(wallSceneryType, String::printf(TXT("wall")));
				wallScenery->init(_room->get_in_sub_world());
			});

			Transform actualPlacement = wallPlacement->placement;
			Vector3 edge = actualPlacement.get_translation();
			edge.z += rg.get(wallHeightAbove) + topOffset;
			if (info->wallsOfSameHeight)
			{
				edge.z = unifiedZ;
			}
			actualPlacement.set_translation(edge);
			actualPlacement.set_translation(actualPlacement.get_translation() + actualPlacement.vector_to_world(info->wallOffsetLocation));

			wallScenery->access_variables().access<Transform>(NAME(wallEdgeTop)) = actualPlacement;

			wallScenery->access_individual_random_generator() = rg.spawn();
			wallScenery->access_individual_random_generator().advance_seed(834597, 235); // to get same result

			_room->collect_variables(wallScenery->access_variables());

			info->apply_generation_parameters_to(wallScenery->access_variables());

			wallScenery->initialise_modules();

			_game->perform_sync_world_job(TXT("place wall"), [&wallScenery, _room]()
			{
				wallScenery->get_presence()->place_in_room(_room, Transform::identity);
			});

			_game->on_newly_created_object(wallScenery);

			todo_hack(TXT("maybe we should never do this? or somewhere else? DO WHAT?"));
		}
	}

	return result;
}

