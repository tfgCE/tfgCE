#include "useMeshes.h"

#include "..\roomGenerationInfo.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\environmentInfo.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

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

REGISTER_FOR_FAST_CAST(UseMeshesInfo);

UseMeshesInfo::UseMeshesInfo()
{
}

UseMeshesInfo::~UseMeshesInfo()
{
}

bool UseMeshesInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("add")))
	{
		if (node->has_attribute(TXT("mesh")))
		{
			UseMesh um;
			um.mesh.load_from_xml(node, TXT("mesh"), _lc);
			um.placement.load_from_xml_child_node(node, TXT("placement"));
			useMeshes.push_back(um);
		}
		else if (node->has_attribute(TXT("sceneryType")))
		{
			UseScenery us;
			us.sceneryType.load_from_xml(node, TXT("sceneryType"), _lc);
			us.placement.load_from_xml_child_node(node, TXT("placement"));
			useSceneries.push_back(us);
		}
		else
		{
			error_loading_xml(node, TXT("not recognised"));
			result = false;
		}
	}

	return result;
}

bool UseMeshesInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(um, useMeshes)
	{
		result &= um->mesh.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	for_every(us, useSceneries)
	{
		result &= us->sceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}

bool UseMeshesInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	for_every(um, useMeshes)
	{
		_renamer.apply_to(um->mesh);
	}

	for_every(us, useSceneries)
	{
		_renamer.apply_to(us->sceneryType);
	}

	return result;
}

Framework::RoomGeneratorInfoPtr UseMeshesInfo::create_copy() const
{
	UseMeshesInfo * copy = new UseMeshesInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool UseMeshesInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	UseMeshes sr(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("meshes (room) \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= sr.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

UseMeshes::UseMeshes(UseMeshesInfo const * _info)
: info(_info)
{
}

UseMeshes::~UseMeshes()
{
}

bool UseMeshes::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;
	auto roomGenerationInfo = RoomGenerationInfo::get();

#ifdef OUTPUT_GENERATION
	output(TXT("generating room using meshes %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif

	bool doorsHaveVRPlacementSet = true;

	for_every_ptr(door, _room->get_all_doors())
	{
		if (!door->is_vr_space_placement_set())
		{
			doorsHaveVRPlacementSet = false;
		}
	}

	if (!doorsHaveVRPlacementSet)
	{
		error(TXT("do not use this generator if there are doors without vr placement set! we may move door but we won't set vr placement"));
		return false;
	}
	else
	{
		for_every(um, info->useMeshes)
		{
			_room->add_mesh(um->mesh.get(), um->placement);
		}
		for_every(us, info->useSceneries)
		{
			auto randomGenerator = _room->get_individual_random_generator();
			randomGenerator.advance_seed(1282 + for_everys_index(us), 179923);

			us->sceneryType->load_on_demand_if_required();

			Framework::Scenery* scenery = nullptr;
			_game->perform_sync_world_job(TXT("spawn scenery"), [&scenery, us, _room]()
			{
				scenery = new Framework::Scenery(us->sceneryType.get(), String::printf(TXT("scenery")));
				scenery->init(_room->get_in_sub_world());
			});

			scenery->access_individual_random_generator() = randomGenerator.spawn();

			_room->collect_variables(scenery->access_variables());

			info->apply_generation_parameters_to(scenery->access_variables());

			scenery->initialise_modules();

			_game->perform_sync_world_job(TXT("place scenery"), [&scenery, _room, us]()
			{
				scenery->get_presence()->place_in_room(_room, us->placement);
			});

			_game->on_newly_created_object(scenery);
		}
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated room using meshes"));
#endif

	if (!_subGenerator)
	{
		_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	// otherwise first try to use existing vr door placement - if it is impossible, try dropping vr placement on more and more pieces
	return result;
}
