#include "simpleElevatorWall.h"

#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
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

REGISTER_FOR_FAST_CAST(SimpleElevatorWallInfo);

SimpleElevatorWallInfo::SimpleElevatorWallInfo()
{
}

SimpleElevatorWallInfo::~SimpleElevatorWallInfo()
{
}

bool SimpleElevatorWallInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= wallSceneryType.load_from_xml(_node, TXT("wallSceneryType"));

	_lc.load_group_into(wallSceneryType);

	return result;
}

bool SimpleElevatorWallInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool SimpleElevatorWallInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr SimpleElevatorWallInfo::create_copy() const
{
	SimpleElevatorWallInfo * copy = new SimpleElevatorWallInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool SimpleElevatorWallInfo::generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	SimpleElevatorWall sr(this);
	result &= sr.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

SimpleElevatorWall::SimpleElevatorWall(SimpleElevatorWallInfo const * _info)
: info(_info)
{
}

SimpleElevatorWall::~SimpleElevatorWall()
{
}

bool SimpleElevatorWall::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;

	SimpleVariableStorage variables;
	_room->collect_variables(variables);
	info->apply_generation_parameters_to(variables);

	if (auto * wallSceneryType = Framework::Library::get_current()->get_scenery_types().find(info->wallSceneryType.get(variables)))
	{
		if (_room->get_all_doors().get_size() == 2)
		{
			auto randomGenerator = _room->get_individual_random_generator();
			randomGenerator.advance_seed(1282, 179923);

			wallSceneryType->load_on_demand_if_required();

			Framework::Scenery* wallScenery = nullptr;
			_game->perform_sync_world_job(TXT("spawn scenery"), [&wallScenery, wallSceneryType, _room]()
			{
				wallScenery = new Framework::Scenery(wallSceneryType, String::printf(TXT("wall")));
				wallScenery->init(_room->get_in_sub_world());
			});

			Transform doorPlacementLower;
			Transform doorPlacementUpper;
			doorPlacementLower = _room->get_all_doors()[0]->get_placement();
			doorPlacementUpper = _room->get_all_doors()[1]->get_placement();

			if (doorPlacementLower.get_translation().z > doorPlacementUpper.get_translation().z)
			{
				swap(doorPlacementLower, doorPlacementUpper);
			}

			float wallHoleWidth = 1.0f;
			Transform wallEdgeTop(doorPlacementUpper.get_translation() + Vector3::zAxis * 10.0f, doorPlacementLower.get_orientation());
			if (auto* elevatorEdgeTopParam = _room->get_variables().get_existing<Vector3>(NAME(elevatorEdgeTop)))
			{
				wallEdgeTop.set_translation(*elevatorEdgeTopParam);
			}
			if (auto* elevatorWidthParam = _room->get_variables().get_existing<float>(NAME(elevatorWidth)))
			{
				wallHoleWidth = *elevatorWidthParam;
			}
			
			float wallHoleLower = wallEdgeTop.get_translation().z;
			float wallHoleUpper = wallEdgeTop.get_translation().z;

			wallHoleLower = doorPlacementLower.get_translation().z;
			wallHoleUpper = doorPlacementUpper.get_translation().z;

			wallScenery->access_individual_random_generator() = randomGenerator.spawn();
			wallScenery->access_variables().access<Transform>(NAME(wallEdgeTop)) = wallEdgeTop;
			wallScenery->access_variables().access<float>(NAME(wallHoleWidth)) = wallHoleWidth;
			wallScenery->access_variables().access<float>(NAME(wallHoleLower)) = wallHoleLower;
			wallScenery->access_variables().access<float>(NAME(wallHoleUpper)) = wallHoleUpper;

			_room->collect_variables(wallScenery->access_variables());

			info->apply_generation_parameters_to(wallScenery->access_variables());

			wallScenery->initialise_modules();

			_game->perform_sync_world_job(TXT("place wall"), [&wallScenery, _room]()
			{
				wallScenery->get_presence()->place_in_room(_room, Transform::identity);
			});

			_game->on_newly_created_object(wallScenery);
		}
		else
		{
			error(TXT("simple elevator room requires two doors"));
			result = false;
		}
	}

	if (!_subGenerator)
	{
		_room->mark_mesh_generated();
	}

	return result;
}

