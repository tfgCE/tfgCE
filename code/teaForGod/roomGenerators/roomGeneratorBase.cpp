#include "roomGeneratorBase.h"

#include "..\teaForGod.h"
#include "..\utils.h"
#include "roomGeneratorUtils.h"

#include "..\..\framework\game\game.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\object\object.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

DEFINE_STATIC_NAME(emissiveColour);
DEFINE_STATIC_NAME(emissivePower);
DEFINE_STATIC_NAME(setEmissiveFromEnvironment);
DEFINE_STATIC_NAME(allowEmissiveFromEnvironment);
DEFINE_STATIC_NAME(zeroEmissive);

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

bool SpawnObjectInfo::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	objectTypeName.load_from_xml(_node, TXT("objectType"), _lc);
	objectTypesTagged.load_from_xml_attribute(_node, TXT("objectTypesTagged"));
	amount.load_from_xml(_node, TXT("amount"));
	tagSpawned.load_from_xml_attribute_or_child_node(_node, TXT("tag"));

	placement.load_from_xml_child_node(_node, TXT("placement"));
	placeAtPOI = _node->get_name_attribute_or_from_child(TXT("placeAtPOI"), placeAtPOI);

	shareMesh = _node->get_bool_attribute_or_from_child_presence(TXT("shareMesh"), shareMesh);
	shareMeshLimit = _node->get_int_attribute_or_from_child(TXT("shareMeshLimit"), shareMeshLimit);

	return result;
}

bool SpawnObjectInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		objectTypes.clear();
		if (!objectTypesTagged.is_empty())
		{
			for_every_ptr(ot, _library->get_actor_types().get_tagged(objectTypesTagged))
			{
				objectTypes.push_back(Framework::UsedLibraryStored<Framework::ObjectType>(ot));
			}
			for_every_ptr(ot, _library->get_scenery_types().get_tagged(objectTypesTagged))
			{
				objectTypes.push_back(Framework::UsedLibraryStored<Framework::ObjectType>(ot));
			}
			for_every_ptr(ot, _library->get_item_types().get_tagged(objectTypesTagged))
			{
				objectTypes.push_back(Framework::UsedLibraryStored<Framework::ObjectType>(ot));
			}
		}
		if (objectTypes.is_empty()) { if (auto objectType = _library->get_actor_types().find_may_fail(objectTypeName)) { objectTypes.push_back(Framework::UsedLibraryStored<Framework::ObjectType>(objectType)); } }
		if (objectTypes.is_empty()) { if (auto objectType = _library->get_scenery_types().find_may_fail(objectTypeName)) { objectTypes.push_back(Framework::UsedLibraryStored<Framework::ObjectType>(objectType)); } }
		if (objectTypes.is_empty()) { if (auto objectType = _library->get_item_types().find_may_fail(objectTypeName)) { objectTypes.push_back(Framework::UsedLibraryStored<Framework::ObjectType>(objectType)); } }
		if (objectTypes.is_empty() && objectTypeName.is_valid())
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (!Framework::Library::may_ignore_missing_references())
#endif
			{
				error(TXT("couldn't find objectType \"%S\""), objectTypeName.to_string().to_char());
				result = false;
			}
		}
		else if (objectTypes.is_empty())
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (!Framework::Library::may_ignore_missing_references())
#endif
			{
				error(TXT("couldn't find objectTypes for SpawnObjectInfo"));
				result = false;
			}
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(Base);

Base::Base()
{
	setEmissiveFromEnvironment.set_param(NAME(setEmissiveFromEnvironment));
	allowEmissiveFromEnvironment.set_param(NAME(allowEmissiveFromEnvironment));
	zeroEmissive.set_param(NAME(zeroEmissive));
}

Base::~Base()
{
}

bool Base::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	wallOffset = _node->get_float_attribute_or_from_child(TXT("wallOffset"), wallOffset);

	chooseRandomBackgroundRoomGenerator = _node->get_bool_attribute_or_from_child_presence(TXT("chooseRandomBackgroundRoomGenerator"), chooseRandomBackgroundRoomGenerator);

	result &= backgroundRoomGeneratorInfo.load_from_xml(_node, TXT("backgroundRoomGenerators"), TXT("backgroundRoomGenerator"), _lc);

	result &= setEmissiveFromEnvironment.load_from_xml(_node, TXT("setEmissiveFromEnvironment"));
	result &= zeroEmissive.load_from_xml(_node, TXT("zeroEmissive"));
	
	result &= aiManagers.load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("spawn")))
	{
		SpawnObjectInfo soi;
		if (soi.load_from_xml(node, _lc))
		{
			spawnObjectsInfo.push_back(soi);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool Base::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= backgroundRoomGeneratorInfo.prepare_for_game(_library, _pfgContext);

	result &= aiManagers.prepare_for_game(_library, _pfgContext);

	for_every(soi, spawnObjectsInfo)
	{
		result &= soi->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool Base::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	result &= backgroundRoomGeneratorInfo.apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr Base::create_copy() const
{
	an_assert(false, TXT("can't create copy of base, do you miss create_copy in subclass?"));
	return Framework::RoomGeneratorInfoPtr(nullptr);
}

#define COPY_FROM_PARAM_OR_ROOM_TO_ROOM(what, defaultValue) \
	{ \
		bool val = defaultValue; \
		val = _room->get_value<bool>(NAME(what), val); /* override with param if set */ \
		val = what.get(_room, val, AllowToFail); \
		_room->access_variables().access<bool>(NAME(what)) = val; \
	} \

bool Base::generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	result &= internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);

	{
		Random::Generator rg = _room->get_individual_random_generator();
		rg.advance_seed(3798, 20934);

		if (chooseRandomBackgroundRoomGenerator)
		{
			// just one
			if (auto * rgi = backgroundRoomGeneratorInfo.get(rg))
			{
				result &= rgi->apply_generation_parameters_to(_room);
				result &= rgi->generate(_room, true, REF_ _roomGeneratingContext);
			}
		}
		else
		{
			// generate all
			for_count(int, rgIdx, backgroundRoomGeneratorInfo.get_count())
			{
				if (auto* rgi = backgroundRoomGeneratorInfo.get(rgIdx))
				{
					result &= rgi->apply_generation_parameters_to(_room);
					result &= rgi->generate(_room, true, REF_ _roomGeneratingContext);
				}
			}
		}
	}

	result &= base::generate(_room, _subGenerator, REF_ _roomGeneratingContext);

	// emissives handled in post_generate

	result &= aiManagers.add_to_room(_room, &get_generation_parameters());

	return result;
}

bool Base::post_generate(Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	result &= base::post_generate(_room, REF_ _roomGeneratingContext);

	// copy values to reside in room
	// use param
	// but if set right in this room (when creating), use that value
	COPY_FROM_PARAM_OR_ROOM_TO_ROOM(setEmissiveFromEnvironment, DEFAULT_SET_EMISSIVE_FROM_ENVIRONMENT);
	COPY_FROM_PARAM_OR_ROOM_TO_ROOM(allowEmissiveFromEnvironment, DEFAULT_ALLOW_EMISSIVE_FROM_ENVIRONMENT);
	COPY_FROM_PARAM_OR_ROOM_TO_ROOM(zeroEmissive, DEFAULT_ZERO_EMISSIVE_FROM_ENVIRONMENT);

	RoomGeneratorUtils::alter_environments_light_for(_room);

	spawn_objects(_room, _roomGeneratingContext);

	Utils::set_emissives_for_room_and_all_inside(_room);

	return result;
}

void Base::spawn_objects(Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	if (spawnObjectsInfo.is_empty())
	{
		return;
	}

	Random::Generator random = _room->get_individual_random_generator();
	random.advance_seed(23088063, 252395);

#ifdef OUTPUT_GENERATION
	output(TXT("spawning objects related to this room generator, for room \"%S\""), _room->get_name().to_char());
#endif

	for_every(soi, spawnObjectsInfo)
	{
		Framework::Mesh const* sharedMesh = nullptr;
		int sharedMeshCount = 0;

		int amount = soi->amount.get(_room, 1);
		for_count(int, i, amount)
		{
			if (soi->whenToSpawn == SpawnObjectInfo::WhenToSpawn::OnRoomGeneration &&
				! soi->objectTypes.is_empty())
			{
				if (auto* game = Framework::Game::get())
				{
					if (Framework::ObjectType* objectType = soi->objectTypes[random.get_int(soi->objectTypes.get_size())].get())
					{
						objectType->load_on_demand_if_required();
						Framework::Object* spawnedObject = nullptr;
						game->perform_sync_world_job(TXT("spawn object"), [objectType, &spawnedObject, _room]()
							{
								spawnedObject = objectType->create(String::printf(TXT("via room generator base")));
								spawnedObject->init(_room->get_in_sub_world());
							});
						spawnedObject->access_individual_random_generator() = random.spawn();
						spawnedObject->access_tags().set_tags_from(soi->tagSpawned);
						_room->collect_variables(spawnedObject->access_variables());
						spawnedObject->initialise_modules([&sharedMesh, soi](Framework::Module* _module)
							{
								if (soi->shareMesh)
								{
									if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
									{
										if (sharedMesh)
										{
											moduleAppearance->use_mesh(sharedMesh);
										}
									}
								}
							});
						Transform placement = soi->placement;
						if (soi->placeAtPOI.is_valid())
						{
							Framework::PointOfInterestInstance* foundPOI = nullptr;
							if (_room->find_any_point_of_interest(soi->placeAtPOI, foundPOI, true, &random))
							{
								if (foundPOI)
								{
									placement = foundPOI->calculate_placement();
								}
							}
						}
						game->perform_sync_world_job(TXT("place object"), [&spawnedObject, _room, placement]()
							{
								spawnedObject->get_presence()->place_in_room(_room, placement);
							});
						if (auto* moduleAppearance = spawnedObject->get_appearance())
						{
							if (soi->shareMesh)
							{
								if (!sharedMesh)
								{
									moduleAppearance->access_mesh()->be_shared();
									sharedMesh = moduleAppearance->get_mesh();
								}
								++sharedMeshCount;
								if (sharedMeshCount >= soi->shareMeshLimit)
								{
									sharedMesh = nullptr;
									sharedMeshCount = 0;
								}
							}
						}
					}
				}
			}
			else
			{
				todo_implement;
			}
		}
	}
}

bool Base::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	return true;
}
