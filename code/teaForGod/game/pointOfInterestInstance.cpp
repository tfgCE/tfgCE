#include "pointOfInterestInstance.h"

#include "..\ai\managerDatas\aiManagerData_spawnSet.h"

#include "..\utils.h"
#include "..\game\gameSettings.h"

#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\framework\game\game.h"
#include "..\..\framework\object\object.h"
#include "..\..\framework\world\room.h"
#include "..\..\framework\world\worldAddress.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		//#define INSPECT_SPAWNING
	#endif
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(spawnSet);

//

// poi function
DEFINE_STATIC_NAME_STR(functionAddOpenWorldBackgroundScenery, TXT("add open world background scenery"));
DEFINE_STATIC_NAME_STR(functionViewOntoVistaSceneryRoom, TXT("view onto vista scenery room"));
DEFINE_STATIC_NAME_STR(functionSpawn, TXT("spawn"));
DEFINE_STATIC_NAME_STR(functionFuseMesh, TXT("fuse mesh"));

// poi params
DEFINE_STATIC_NAME(storeExistenceInPilgrimage);
DEFINE_STATIC_NAME(storeExistenceInPilgrimageWorldAddress);
DEFINE_STATIC_NAME(storeExistenceInPilgrimagePOIIdx);

// object type tag
DEFINE_STATIC_NAME(hostileCharacter);

// room/region parameters
DEFINE_STATIC_NAME(gameDirectorSafeSpace);

//

void PointOfInterestInstance::setup_function_spawn(OUT_ Framework::ObjectType * & _objectType, OUT_ Framework::TemporaryObjectType * & _temporaryObjectType)
{
	base::setup_function_spawn(_objectType, _temporaryObjectType);
	if (Name const * spawnSet = get_parameters().get_existing<Name>(NAME(spawnSet)))
	{
		if (auto* ot = AI::ManagerDatasLib::SpawnSet::choose(*spawnSet, nullptr, get_room(), access_random_generator()))
		{
			_objectType = ot;
		}
	}
	if (_objectType && _objectType->get_tags().get_tag(NAME(hostileCharacter)) &&
		(GameSettings::get().difficulty.npcs <= GameSettings::NPCS::NoHostiles ||
		 get_room()->get_value<bool>(NAME(gameDirectorSafeSpace), false, false)))
	{
		_objectType = nullptr;
	}
}

void PointOfInterestInstance::process_function(Name const& function)
{
	if (function == NAME(functionAddOpenWorldBackgroundScenery))
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			::SafePtr<Framework::Room> useRoom(get_room());

			std::function<void()> add_scenery = nullptr;

			Transform placeAt = calculate_placement();
			add_scenery = [useRoom, piow, placeAt]()
			{
				piow->add_scenery_in_room(useRoom.get(), NP, placeAt);
			};

			{
				// check if we should do this async or not
				// if we're not active, we should do this immediately
				// if we're active, spawn async job
				bool doAsync = false;
				if (auto* o = fast_cast<Framework::Object>(owner.get()))
				{
					doAsync |= o->is_world_active();
				}
				if (!owner.get() && useRoom.get() && useRoom->is_world_active())
				{
					doAsync |= true;
				}
				if (doAsync)
				{
					Framework::Game::get()->add_async_world_job(TXT("async process"), add_scenery);
				}
				else
				{
					add_scenery();
				}
			}
		}
	}
	else if (function == NAME(functionViewOntoVistaSceneryRoom))
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			::SafePtr<Framework::Room> useRoom(get_room());

			auto optCellAt = piow->find_cell_at(useRoom.get());
			if (optCellAt.is_set())
			{
				useRoom->set_room_for_rendering_additional_objects(piow->get_vista_scenery_room(optCellAt.get()));
			}
		}
	}
	else
	{
		if (function == NAME(functionSpawn))
		{
			if (parameters.get_value<bool>(NAME(storeExistenceInPilgrimage), false))
			{
				if (poiIdx == NONE)
				{
					warn(TXT("poiIdx required, might be not supported by IMOs and room parts"));
				}
				else
				{
					::SafePtr<Framework::Room> useRoom(get_room());

					if (useRoom.get())
					{
						auto& wa = parameters.access<Framework::WorldAddress>(NAME(storeExistenceInPilgrimageWorldAddress));
						auto& waPOIIdx = parameters.access<int>(NAME(storeExistenceInPilgrimagePOIIdx));
						wa.build_for(useRoom.get());
						waPOIIdx = poiIdx;

#ifdef INSPECT_SPAWNING
						output(TXT("poi: check address=\"%S\" poiIdx=\"%i\""), wa.to_string().to_char(), waPOIIdx);
#endif
						if (auto* piow = PilgrimageInstance::get())
						{
#ifdef INSPECT_SPAWNING
							output(TXT("check in pilgrimage : %S"), piow->has_been_killed(wa, waPOIIdx) ? TXT("killed") : TXT("ok"));
#endif
							if (piow->has_been_killed(wa, waPOIIdx))
							{
								// skip, already killed
#ifdef INSPECT_SPAWNING
								output(TXT("object already killed, do not spawn"));
#endif
								return;
							}
						}
					}
				}
			}
		}
		base::process_function(function);
		if (function == NAME(functionFuseMesh))
		{
			if (auto* iOwner = owner.get())
			{
				// the mesh has changed, we need to set emissives again (if they are required)
				Utils::set_emissives_from_room_to(iOwner);
			}
		}
	}
}
