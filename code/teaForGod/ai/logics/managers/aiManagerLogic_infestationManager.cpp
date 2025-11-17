#include "aiManagerLogic_infestationManager.h"

#include "..\..\..\ai\aiManagerUtils.h"
#include "..\..\..\game\game.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\custom\mc_pilgrimageDevice.h"
#include "..\..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\actor.h"

#include "..\..\..\..\core\other\packedBytes.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// variables
DEFINE_STATIC_NAME(blobsHealth);

//

REGISTER_FOR_FAST_CAST(InfestationManager);

InfestationManager::InfestationManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	infestationManagerData = fast_cast<InfestationManagerData>(_logicData);
}

InfestationManager::~InfestationManager()
{
}

void InfestationManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

void InfestationManager::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(InfestationManager::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai infestationManager] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Optional<VectorInt2>, cellAt);
	
	LATENT_VAR(Random::Generator, spawnRG);
	LATENT_VAR(Framework::SceneryType*, spawnInfestationBlobType);
	
	LATENT_VAR(bool, blobSpawned);

	LATENT_VAR(int, latentI);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<InfestationManager>(logic);

	LATENT_BEGIN_CODE();

	self->depleted = false;

	ai_log(self, TXT("infestationManager box, hello!"));

	{
		self->blobsHealthVar.set_name(NAME(blobsHealth));
		self->blobsHealthVar.look_up<PackedBytes>(imo->access_variables());
	}

	if (imo)
	{
		{
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				cellAt = piow->find_cell_at(imo);
				if (cellAt.is_set())
				{
					self->depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo);
					ai_log(self, self->depleted? TXT("depleted") : TXT("available"));
				}
			}
			if (!cellAt.is_set())
			{
				error_dev_investigate(TXT("no cell found, this should not happen and definitely won't work"));
			}
		}
		LATENT_YIELD();
		{
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				if (cellAt.is_set())
				{
					self->mainRegion = piow->get_main_region_for_cell(cellAt.get());
				}
			}
			if (!self->mainRegion)
			{
				// fallback, for test room
				self->mainRegion = imo->get_presence()->get_in_room()->get_in_region();
			}
		}
	}

	self->rg = Random::Generator(0, 0);
	if (cellAt.is_set())
	{
		self->rg.advance_seed(cellAt.get().x, cellAt.get().y);
	}

	if (self->infestationManagerData->infestationBlobs.is_empty())
	{
		self->infestationBlobsCount = 0;
	}
	else
	{
		// order infestation blobs to spawn, we use rg which is constant through loading/saving
		self->infestationBlobsCount = self->rg.get(self->infestationManagerData->infestationBlobsCount);
		latentI = 0;
		while (latentI < self->infestationBlobsCount)
		{
			// use spawnRG as we may have different situation depending on the map
			spawnRG = self->rg.spawn();

			// choose type to spawn
			spawnInfestationBlobType = self->infestationManagerData->infestationBlobs[spawnRG.get_int(self->infestationManagerData->infestationBlobs.get_size())].get();

			blobSpawned = false;
			while (!blobSpawned)
			{
				// find place to spawn, get random spawn point from the region
				{
					Framework::PointOfInterestInstance* foundPOI = nullptr;
					if (self->mainRegion->find_any_point_of_interest(self->infestationManagerData->infestationBlobSpawnPointPOI, foundPOI, &spawnRG,
						[self](Framework::PointOfInterestInstance const* _poi) { return self->infestationManagerData->infestationBlobSpawnPointTagged.check(_poi->get_tags()); }))
					{
						if (auto* r = foundPOI->get_room())
						{
							// trace to find the right placement, try the same room
							Transform poiPlacement = foundPOI->calculate_placement();
							Vector3 startLoc = Vector3::zAxis * 0.5f;
							startLoc = poiPlacement.location_to_world(startLoc);
							Vector3 endLoc;
							endLoc.x = spawnRG.get_float(-1.0f, 1.0f);
							endLoc.y = spawnRG.get_float(-1.0f, 1.0f);
							endLoc.z = spawnRG.get_float(-1.0f, 1.0f);
							endLoc = startLoc + endLoc.normal() * 3.0f;

							// raycast
							Framework::CheckCollisionContext checkCollisionContext;
							checkCollisionContext.collision_info_needed();
							checkCollisionContext.start_with_nothing_to_check();
							checkCollisionContext.check_scenery();
							checkCollisionContext.check_room();
							checkCollisionContext.check_room_scenery();

							checkCollisionContext.use_collision_flags(self->infestationManagerData->infestationBlobPlantCollisionFlags);

							Segment segment(startLoc, endLoc);

							Framework::CheckSegmentResult hitResult;
							Framework::Room const* endsInRoom = nullptr;
							Transform intoEndRoomTransform;

							if (r->check_segment_against(AgainstCollision::Movement, REF_ segment, hitResult, checkCollisionContext, endsInRoom, intoEndRoomTransform))
							{
								// we hit something, we want to be in the same room
								if (!hitResult.changedRoom && endsInRoom == r)
								{
									PackedBytes const & blobsHeatlh = self->blobsHealthVar.access<PackedBytes>();
									if (!blobsHeatlh.is_index_valid(latentI) ||
										blobsHeatlh.get(latentI) > 0)
									{
										Vector3 forward = Rotator3(0.0f, spawnRG.get_float(-180.0f, 180.0f), 0.0f).get_forward();
										if (abs(Vector3::dot(forward, hitResult.hitNormal)) > 0.9f)
										{
											swap(forward.x, forward.y);
										}
										Transform placement;
										placement.build_from_two_axes(Axis::Up, Axis::Forward, hitResult.hitNormal, forward, hitResult.hitLocation);

										// create DOC
										Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
										doc->activateImmediatelyEvenIfRoomVisible = true;
										doc->wmpOwnerObject = imo;
										doc->inRoom = hitResult.inRoom;
										doc->name = String::printf(TXT("infestation blob %i"), latentI);
										doc->objectType = spawnInfestationBlobType;
										doc->randomGenerator = spawnRG;
										doc->priority = 1000;
										doc->checkSpaceBlockers = false;
										doc->placement = placement;

										hitResult.inRoom->collect_variables(doc->variables);

										Game::get()->queue_delayed_object_creation(doc);

										self->infestationBlobDOCs.push_back();
										self->infestationBlobDOCs.get_last() = doc;
									}
									else
									{
										// empty to have the process properly and fully working
										self->infestationBlobDOCs.push_back();
									}

									blobSpawned = true;
								}
							}
						}
					}
				}
				LATENT_YIELD();
			};

			++latentI;
			LATENT_YIELD();
		}
	}

	if (!self->depleted)
	{
		latentI = 0;
		while (latentI < self->infestationManagerData->aiManagerMinds.get_size())
		{
			self->queue_spawn_ai_manager(latentI);
			++latentI;
			LATENT_YIELD();
		}
	}

	while (true)
	{
		if (! self->handle_blobs())
		{
			// all dead!
			if (!self->depleted)
			{
				// wait a bit so the last explosion ends
				LATENT_WAIT(2.0f);

				{
					self->depleted = true;
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						if (cellAt.is_set())
						{
							piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
						}
					}
					if (auto* ms = MissionState::get_current())
					{
						ms->stopped_infestation();
					}
				}
			}
		}

		self->handle_ai_managers(!self->depleted);

		LATENT_WAIT(Random::get_float(0.05f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

bool InfestationManager::handle_blobs()
{
	bool atLeastOneAlive = false;

	PackedBytes& blobsHealth = blobsHealthVar.access<PackedBytes>();

	// get only first, one by one, in order
	if (! infestationBlobDOCs.is_empty())
	{
		atLeastOneAlive = true; // not alive... yet!

		if (auto* doc = infestationBlobDOCs.get_first().get())
		{
			if (doc->is_done())
			{
				int blobIdx = infestationBlobs.get_size();

				infestationBlobs.push_back(doc->createdObject);

				infestationBlobDOCs.pop_front();

				if (auto* h = doc->createdObject->get_custom<CustomModules::Health>())
				{
					// check if we have any former records
					if (blobsHealth.is_index_valid(blobIdx))
					{
						// alright, restore health from what we stored
						float hPt = (float)blobsHealth.get(blobIdx) / 255.0f;
						h->set_health(h->get_max_health().mul(hPt));
					}
					else
					{
						// create a record now, hPt should be 1.0 here but who knows!
						float hPt = clamp(h->get_health().div_to_float(h->get_max_health()), 0.0f, 1.0f);
						hPt = max(0.05f, hPt);
						byte hByte = (byte)(hPt * 255.0f);
						blobsHealth.set_size(blobIdx + 1);
						blobsHealth.set(blobIdx, hByte);
					}
				}
			}
		}
		else
		{
			infestationBlobs.push_back();
			infestationBlobDOCs.pop_front();
		}
	}

	Framework::IModulesOwner* firstAliveBlob = nullptr;
	for_every(blobRef, infestationBlobs)
	{
		byte hByte = 0;
		auto* blobIMO = blobRef->get();
		int blobIdx = for_everys_index(blobRef);

		// read from actual health, if exists, if not, we will assume this one's died
		if (blobIMO)
		{
			if (auto* h = blobIMO->get_custom<CustomModules::Health>())
			{
				float hPt = h->get_health().div_to_float(h->get_max_health());
				hPt = clamp(hPt, 0.0f, 1.0f);

				if (hPt > 0.0f)
				{
					atLeastOneAlive = true;
					if (!firstAliveBlob)
					{
						firstAliveBlob = blobIMO;
					}
				}

				hByte = (byte)(hPt * 255.0f);
				if (hByte == 0 && hPt > 0.0f)
				{
					// 0 is only if really killed/dead
					hByte = 1;
				}
			}
		}

		// update record
		{
			if (blobIdx >= blobsHealth.get_size())
			{
				// there is no such entry, add it
				blobsHealth.set_size(blobIdx + 1);
			}
			blobsHealth.set(blobIdx, hByte);
		}
	}

	if (guideTowardInfestationBlob != firstAliveBlob)
	{
		guideTowardInfestationBlob = firstAliveBlob;

		if (auto* imo = get_mind()->get_owner_as_modules_owner())
		{
			if (auto* mpd = imo->get_custom<CustomModules::PilgrimageDevice>())
			{
				// if changes, will also inform pilgrimage instance
				mpd->set_guide_towards(guideTowardInfestationBlob.get());
			}
		}
	}

	return atLeastOneAlive;
}

void InfestationManager::queue_spawn_ai_manager(int _idx)
{
	an_assert(aiManagers.is_empty() && aiManagerDOCs.is_empty());

	if (infestationManagerData->aiManagerMinds.is_index_valid(_idx))
	{
		if (auto* aiMind = infestationManagerData->aiManagerMinds[_idx].get())
		{
			auto* imo = get_mind()->get_owner_as_modules_owner();
			auto* inRoom = imo->get_presence()->get_in_room();

			RefCountObjectPtr<Framework::DelayedObjectCreation> doc;

			if (AI::ManagerUtils::queue_spawn_ai_manager(REF_ doc,
					AI::ManagerUtils::SpawnAiManagerParams()
						.with_mind(aiMind)
						.for_owner(imo)
						.for_region(mainRegion)
						.in_room(inRoom)))
			{
				aiManagerDOCs.push_back(doc);
			}
		}
	}
}

void InfestationManager::handle_ai_managers(bool _shouldExist)
{
	// any order of processing is fine
	for_every_ref(doc, aiManagerDOCs)
	{
		if (doc->is_done())
		{
			aiManagers.push_back(doc->createdObject);
			aiManagerDOCs.remove_at(for_everys_index(doc));
			break;
		}
	}

	if (!_shouldExist)
	{
		for_every_ref(imo, aiManagers)
		{
			imo->cease_to_exist(false);
		}
		aiManagers.clear();
	}
}

//

REGISTER_FOR_FAST_CAST(InfestationManagerData);

bool InfestationManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	aiManagerMinds.clear();
	infestationBlobs.clear();
	infestationBlobsCount = RangeInt(3, 4);
	infestationBlobSpawnPointPOI = Name::invalid();
	infestationBlobSpawnPointTagged.clear();
	infestationBlobPlantCollisionFlags = Collision::Flags::none();


	// load
	//
	for_every(n, _node->children_named(TXT("aiManager")))
	{
		aiManagerMinds.push_back();
		if (aiManagerMinds.get_last().load_from_xml(n, TXT("aiMind"), _lc))
		{
			// ok
		}
		else
		{
			aiManagerMinds.pop_back();
		}
	}
	for_every(n, _node->children_named(TXT("infestationBlob")))
	{
		infestationBlobs.push_back();
		if (infestationBlobs.get_last().load_from_xml(n, TXT("sceneryType"), _lc))
		{
			// ok
		}
		else
		{
			infestationBlobs.pop_back();
		}
	}
	infestationBlobsCount.load_from_xml(_node, TXT("infestationBlobsCount"));
	infestationBlobSpawnPointPOI = _node->get_name_attribute(TXT("infestationBlobSpawnPointPOI"), infestationBlobSpawnPointPOI);
	infestationBlobSpawnPointTagged.load_from_xml_attribute_or_child_node(_node, TXT("infestationBlobSpawnPointTagged"));
	infestationBlobPlantCollisionFlags.load_from_xml(_node, TXT("infestationBlobPlantCollisionFlags"));

	error_loading_xml_on_assert(infestationBlobSpawnPointPOI.is_valid(), result, _node, TXT("provide \"infestationBlobSpawnPointPOI\""));
	error_loading_xml_on_assert(!infestationBlobPlantCollisionFlags.is_empty(), result, _node, TXT("provide \"infestationBlobPlantCollisionFlags\""));

	error_loading_xml_on_assert(! infestationBlobsCount.is_empty(), result, _node, TXT("\"infestationBlobsCount\" should not provide an empty range"));
	error_loading_xml_on_assert(infestationBlobsCount.max <= PackedBytes::max_size(), result, _node, TXT("\"infestationBlobsCount\" should not exceed %i (or change packed bytes)"), PackedBytes::max_size());

	return result;
}

bool InfestationManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(aiMind, aiManagerMinds)
	{
		result &= aiMind->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	for_every(infestationBlob, infestationBlobs)
	{
		result &= infestationBlob->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}
