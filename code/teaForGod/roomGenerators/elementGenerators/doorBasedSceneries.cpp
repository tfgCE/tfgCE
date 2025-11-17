#include "doorBasedSceneries.h"

#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

DEFINE_STATIC_NAME(beyondDoor_width);
DEFINE_STATIC_NAME(beyondDoor_height);
DEFINE_STATIC_NAME(beyondDoor_depth);
DEFINE_STATIC_NAME(beyondDoor_offsetBeyond);

// unlikely to be used but provide extra context
DEFINE_STATIC_NAME(beyondDoor_allDoors); // range3
DEFINE_STATIC_NAME(beyondDoor_dirOpposite); // door opposite dir
DEFINE_STATIC_NAME(beyondDoor_dirRight); // door opposite dir right (the door might be on left!)
DEFINE_STATIC_NAME(beyondDoor_dirLeft); // door opposite dir left (the door might be on right)
DEFINE_STATIC_NAME(beyondDoor_dirSide); // door opposite dir (left or right)

// global references
DEFINE_STATIC_NAME_STR(grGenericScenery, TXT("generic scenery"));

//

REGISTER_FOR_FAST_CAST(DoorBasedSceneriesInfo);

DoorBasedSceneriesInfo::DoorBasedSceneriesInfo()
{
}

DoorBasedSceneriesInfo::~DoorBasedSceneriesInfo()
{
}

bool DoorBasedSceneriesInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= doorsTagged.load_from_xml_attribute_or_child_node(_node, TXT("doorsTagged"));

	result &= somg.load_from_xml(_node, _lc);

	orderDir.load_from_xml(_node, TXT("orderDir"));
	orderByFurthest = _node->get_bool_attribute_or_from_child(TXT("orderByFurthest"), orderByFurthest);
	unlimitedOrdered = _node->get_bool_attribute_or_from_child_presence(TXT("unlimitedOrdered"), unlimitedOrdered);
	firstIsBackground = _node->get_bool_attribute_or_from_child_presence(TXT("firstIsBackground"), firstIsBackground);
	provideRoomGeneratorContextsSpaceUsedToFirst = _node->get_bool_attribute_or_from_child_presence(TXT("provideRoomGeneratorContextsSpaceUsedToFirst"), provideRoomGeneratorContextsSpaceUsedToFirst);
	for_every(node, _node->children_named(TXT("ordered")))
	{
		SceneryOrMeshGenerator orderedSOMG;
		if (orderedSOMG.load_from_xml(node, _lc))
		{
			orderedSOMGs.push_back(orderedSOMG);
		}
		else
		{
			result = false;
		}
	}

	atSameZMin = _node->get_bool_attribute_or_from_child_presence(TXT("atSameZMin"), atSameZMin);
	atSameZMax = _node->get_bool_attribute_or_from_child_presence(TXT("atSameZMax"), atSameZMax);

	offsetLocation.load_from_xml_child_node(_node, TXT("offsetLocation"));

	groupAll = _node->get_bool_attribute_or_from_child_presence(TXT("groupAll"), groupAll);
	groupByYaw.load_from_xml(_node, TXT("groupByYaw"));
	groupByOffset.load_from_xml(_node, TXT("groupByOffset"));
	if (groupByOffset.is_set() && !groupByYaw.is_set())
	{
		warn_loading_xml(_node, TXT("specified groupByOffset without groupByYaw, assuming value"));
		groupByYaw = 5.0f;
	}
	if (_node->get_bool_attribute_or_from_child_presence(TXT("group")))
	{
		groupByYaw = 5.0f;
		groupByOffset = 0.3f;
	}

	return result;
}

bool DoorBasedSceneriesInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool DoorBasedSceneriesInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr DoorBasedSceneriesInfo::create_copy() const
{
	DoorBasedSceneriesInfo * copy = new DoorBasedSceneriesInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool DoorBasedSceneriesInfo::generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	DoorBasedSceneries sr(this);
	result &= sr.generate(Framework::Game::get(), _room, REF_ _roomGeneratingContext);

	result &= base::generate(_room, _subGenerator, REF_ _roomGeneratingContext);

	if (!_subGenerator)
	{
		_room->mark_mesh_generated();
	}

	return result;
}

//

DoorBasedSceneries::DoorBasedSceneries(DoorBasedSceneriesInfo const * _info)
: info(_info)
{
}

DoorBasedSceneries::~DoorBasedSceneries()
{
}

bool DoorBasedSceneries::generate(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;

	Random::Generator rg = _room->get_individual_random_generator();
	rg.advance_seed(6895, 34597);

	SimpleVariableStorage variables;
	_room->collect_variables(variables);
	info->apply_generation_parameters_to(variables);

	Framework::SceneryType * sceneryType = nullptr;
	Framework::MeshGenerator * meshGenerator = nullptr;
	if (info->somg.sceneryType.is_set())
	{
		sceneryType = Framework::Library::get_current()->get_scenery_types().find(info->somg.sceneryType.get(variables));
	}
	if (info->somg.meshGenerator.is_set())
	{
		meshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->somg.meshGenerator.get(variables));
	}
	struct ActualSceneryOrMeshGenerator
	{
		Framework::SceneryType* sceneryType = nullptr;
		Framework::MeshGenerator* meshGenerator = nullptr;
	};
	ARRAY_STACK(ActualSceneryOrMeshGenerator, orderedSOMGs, info->orderedSOMGs.get_size());
	for_every(somg, info->orderedSOMGs)
	{
		ActualSceneryOrMeshGenerator asomg;
		if (somg->sceneryType.is_set())
		{
			asomg.sceneryType = Framework::Library::get_current()->get_scenery_types().find(somg->sceneryType.get(variables));
		}
		if (somg->meshGenerator.is_set())
		{
			asomg.meshGenerator = Framework::Library::get_current()->get_mesh_generators().find(somg->meshGenerator.get(variables));
		}
		if (asomg.sceneryType || asomg.meshGenerator)
		{
			orderedSOMGs.push_back(asomg);
		}
	}
	struct OrderedPlacement
	{
		Transform placement;
		float ordered;
		float width = 0.0f; // assuming symmetrical
		float height = 0.0f; // assuming symmetrical
		ArrayStatic<Transform, 8> doorPlacements;

		OrderedPlacement()
		{
			SET_EXTRA_DEBUG_INFO(doorPlacements, TXT("PlatformsLinear.dirs (remove)"));
		}
		explicit OrderedPlacement(Transform const& _placement, float _ordered = 0.0f) : placement(_placement), ordered(_ordered) { doorPlacements.push_back(_placement); }
	};
	Array<OrderedPlacement> placements;
	Range zRange = Range::empty;

	{
		for_every_ptr(door, _room->get_all_doors())
		{
			if (!info->doorsTagged.check(door->get_tags()))
			{
				continue;
			}
			Transform placement = door->get_placement();
			bool addNew = true;
			if (info->groupAll && ! placements.is_empty())
			{
				addNew = false;
			}
			else if (info->groupByYaw.is_set() )
			{
				float yaw = placement.get_axis(Axis::Forward).to_rotator().yaw;

				for_every(op, placements)
				{
					float opyaw = op->placement.get_axis(Axis::Forward).to_rotator().yaw;
					if (Rotator3::normalise_axis(yaw - opyaw) <= info->groupByYaw.get())
					{
						if (info->groupByOffset.is_set())
						{
							float offset = op->placement.location_to_local(placement.get_translation()).y;
							if (abs(offset) <= info->groupByOffset.get())
							{
								addNew = false;
								op->doorPlacements.push_back(placement);
								// but keep op's placement as it was
							}
						}
					}
				}
			}
			if (addNew)
			{
				placements.push_back(OrderedPlacement(placement));
				auto size = door->calculate_size();
				placements.get_last().width = size.x.length();
				placements.get_last().height = size.y.max;
			}
		}

		for_every(placement, placements)
		{
			zRange.include(placements.get_first().placement.get_translation().z);
		}

		// reorder
		{
			if (info->orderByFurthest)
			{
				Vector3 centre = Vector3::zero;
				for_every(p, placements)
				{
					centre += p->placement.get_translation();
				}
				centre /= (float)placements.get_size();

				for_every(p, placements)
				{
					// placements are outbound, the lower the value the more important is
					p->ordered = Vector3::dot(p->placement.get_axis(Axis::Forward).normal(), centre - p->placement.get_translation());
				}
			}
			else
			{
				Vector3 orderDir = info->orderDir.get(variables);

				for_every(p, placements)
				{
					p->ordered = Vector3::dot(orderDir, p->placement.get_translation());
				}
			}
			sort(placements, [](const void* _a, const void* _b)
			{
				OrderedPlacement const & a = *(plain_cast<OrderedPlacement>(_a));
				OrderedPlacement const & b = *(plain_cast<OrderedPlacement>(_b));
				if (a.ordered < b.ordered) return A_BEFORE_B;
				if (a.ordered > b.ordered) return B_BEFORE_A;
				return A_AS_B;
			});
		}
	}
	{
		if (info->firstIsBackground)
		{
			placements.push_front(placements.get_first());
		}
		for_every(placement, placements)
		{
			Framework::Scenery* scenery = nullptr;
			Framework::SceneryType* useSceneryType = sceneryType;
			Framework::MeshGenerator* useMeshGenerator = meshGenerator;
			if (!orderedSOMGs.is_empty())
			{
				if (info->unlimitedOrdered)
				{
					// this way we should use use last one if we don't have enough
					useSceneryType = orderedSOMGs[clamp(for_everys_index(placement), 0, orderedSOMGs.get_size() - 1)].sceneryType;
					useMeshGenerator = orderedSOMGs[clamp(for_everys_index(placement), 0, orderedSOMGs.get_size() - 1)].meshGenerator;
				}
				else if (orderedSOMGs.is_index_valid(for_everys_index(placement)))
				{
					useSceneryType = orderedSOMGs[for_everys_index(placement)].sceneryType;
					useMeshGenerator = orderedSOMGs[for_everys_index(placement)].meshGenerator;
				}
				else
				{
					// clear
					useSceneryType = nullptr;
					useMeshGenerator = nullptr;
				}
			}
			if (!useSceneryType && ! useMeshGenerator)
			{
				continue; // just skip
			}
			if (!useSceneryType)
			{
				useSceneryType = Framework::Library::get_current()->get_global_references().get<Framework::SceneryType>(NAME(grGenericScenery));
			}
			if (!useSceneryType)
			{
				error(TXT("no scenery type at this point?"));
				continue;
			}

			useSceneryType->load_on_demand_if_required();
			_game->perform_sync_world_job(TXT("spawn scenery"), [&scenery, useSceneryType, _room]()
			{
				scenery = new Framework::Scenery(useSceneryType, String::printf(TXT("door based scenery")));
				scenery->init(_room->get_in_sub_world());
			});

			Transform actualPlacement = placement->placement;
			Vector3 edge = actualPlacement.get_translation();
			if (info->atSameZMin)
			{
				edge.z = zRange.min;
			}
			if (info->atSameZMax)
			{
				edge.z = zRange.max;
			}
			actualPlacement.set_translation(edge);
			actualPlacement.set_translation(actualPlacement.get_translation() + actualPlacement.vector_to_world(info->offsetLocation));

			scenery->access_individual_random_generator() = rg.spawn();
			scenery->access_individual_random_generator().advance_seed(834597, 235); // to get same result

			_room->collect_variables(scenery->access_variables());

			{
				float bd_width = 0.0f;
				float bd_height = 0.0f;
				// calculate local width and range that includes all other doors, and offset actual placement as we want to have it centered
				if (info->firstIsBackground && for_everys_index(placement) == 0)
				{
					actualPlacement = Transform::identity;
				}
				else
				{
					Range3 thisGroupLocal = Range3::empty;
					for_every(dp, placement->doorPlacements)
					{
						Transform localDP = actualPlacement.to_local(*dp);
						Vector3 x = Vector3::xAxis * (placement->width * 0.5f);
						Vector3 z = Vector3::zAxis * placement->height;
						thisGroupLocal.include(localDP.location_to_world( x));
						thisGroupLocal.include(localDP.location_to_world(-x));
						thisGroupLocal.include(localDP.location_to_world( x + z));
						thisGroupLocal.include(localDP.location_to_world(-x + z));
					}
					actualPlacement = actualPlacement.to_world(Transform(Vector3(thisGroupLocal.x.centre(), 0.0f, thisGroupLocal.z.min), Quat::identity));
					bd_width = thisGroupLocal.x.length();
					bd_height = thisGroupLocal.z.length();
				}
				bool dirOpposite = false;
				bool dirRight = false;
				bool dirLeft = false;
				Range3 allDoorsLocal= Range3::empty;
				for_every(op, placements)
				{
					for_every(dp, op->doorPlacements)
					{
						Transform localDP = actualPlacement.to_local(*dp);
						Vector3 x = Vector3::xAxis * (op->width * 0.5f);
						Vector3 z = Vector3::zAxis * op->height;
						allDoorsLocal.include(localDP.location_to_world(x));
						allDoorsLocal.include(localDP.location_to_world(-x));
						allDoorsLocal.include(localDP.location_to_world(x + z));
						allDoorsLocal.include(localDP.location_to_world(-x + z));
					}
					bool checkDir = op != placement;
					if (info->firstIsBackground && for_everys_index(op) <= 1 && for_everys_index(placement) <= 1)
					{
						// same
						checkDir = false;
					}
					if (checkDir)
					{
						float yaw = Rotator3::normalise_axis(actualPlacement.to_local(op->placement).get_axis(Axis::Forward).to_rotator().yaw);
						if (abs(yaw) > 135.0f)
						{
							dirOpposite = true;
						}
						else if (abs(yaw) > 45.0f)
						{
							if (yaw > 0.0f)
							{
								dirRight = true;
							}
							else
							{
								dirLeft = true;
							}
						}
					}
				}
				if (info->provideRoomGeneratorContextsSpaceUsedToFirst && for_everys_index(placement) == 0 &&
					!_roomGeneratingContext.spaceUsed.is_empty())
				{
					if (Transform::same_with_orientation(actualPlacement, Transform::identity))
					{
						allDoorsLocal.include(_roomGeneratingContext.spaceUsed);
					}
					else
					{
						Range3 spaceUsedLocal = Range3::empty;
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(0.0f, 0.0f, 0.0f))));
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(0.0f, 0.0f, 1.0f))));
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(0.0f, 1.0f, 0.0f))));
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(0.0f, 1.0f, 1.0f))));
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(1.0f, 0.0f, 0.0f))));
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(1.0f, 0.0f, 1.0f))));
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(1.0f, 1.0f, 0.0f))));
						spaceUsedLocal.include(actualPlacement.location_to_local(_roomGeneratingContext.spaceUsed.get_at(Vector3(1.0f, 1.0f, 1.0f))));
						allDoorsLocal.include(spaceUsedLocal);
					}
				}
				scenery->access_variables().access<float>(NAME(beyondDoor_width)) = bd_width;
				scenery->access_variables().access<float>(NAME(beyondDoor_height)) = bd_height;
				scenery->access_variables().access<Range3>(NAME(beyondDoor_allDoors)) = allDoorsLocal;
				scenery->access_variables().access<bool>(NAME(beyondDoor_dirOpposite)) = dirOpposite;
				scenery->access_variables().access<bool>(NAME(beyondDoor_dirRight)) = dirRight;
				scenery->access_variables().access<bool>(NAME(beyondDoor_dirLeft)) = dirLeft;
				scenery->access_variables().access<bool>(NAME(beyondDoor_dirSide)) = dirRight || dirLeft;
			}

			info->apply_generation_parameters_to(scenery->access_variables());

			scenery->initialise_modules([useMeshGenerator](Framework::Module* _module)
			{
				if (useMeshGenerator)
				{
					if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
					{
						moduleAppearance->use_mesh_generator_on_setup(useMeshGenerator);
					}
				}
			});

			_game->perform_sync_world_job(TXT("place scenery"), [&scenery, _room, actualPlacement]()
			{
				scenery->get_presence()->place_in_room(_room, actualPlacement);
			});

			_game->on_newly_created_object(scenery);
		}
	}

	return result;
}

//

bool DoorBasedSceneriesInfo::SceneryOrMeshGenerator::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= sceneryType.load_from_xml(_node, TXT("sceneryType"));
	_lc.load_group_into(sceneryType);
	result &= meshGenerator.load_from_xml(_node, TXT("meshGenerator"));
	_lc.load_group_into(meshGenerator);

	return result;
}
