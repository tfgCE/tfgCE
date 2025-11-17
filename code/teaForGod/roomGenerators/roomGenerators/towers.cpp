#include "towers.h"

#include "..\roomGenerationInfo.h"
#include "..\roomGeneratorUtils.h"

#include "..\..\utils.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\environmentInfo.h"
#include "..\..\..\framework\world\region.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#ifdef AN_CLANG
#include "..\..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
//#define OUTPUT_GENERATION_VARIABLES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

DEFINE_STATIC_NAME(any);
DEFINE_STATIC_NAME(in);
DEFINE_STATIC_NAME(out);
DEFINE_STATIC_NAME(at);
DEFINE_STATIC_NAME(to);
DEFINE_STATIC_NAME(atToDir);
DEFINE_STATIC_NAME(index);
DEFINE_STATIC_NAME(towersForceHalf);
DEFINE_STATIC_NAME(towersMaxSize);
DEFINE_STATIC_NAME(towersMaxTileSize);
DEFINE_STATIC_NAME(anyTower);
DEFINE_STATIC_NAME(towersRadius);
DEFINE_STATIC_NAME(alignToTiles);
DEFINE_STATIC_NAME(towerRadius); // out - whole tower radius, with catwalk
DEFINE_STATIC_NAME(innerTowerRadius); // out - inner tower radius, wall
DEFINE_STATIC_NAME(halfTower); // out - is it half tower (special case for small room)
DEFINE_STATIC_NAME(towersCount);
DEFINE_STATIC_NAME(towersBetweenDoorsCount);

// mesh nodes and mesh nodes variables
DEFINE_STATIC_NAME(tower);
DEFINE_STATIC_NAME(randomSeed);
DEFINE_STATIC_NAME_STR(leftDoor, TXT("left door"));
DEFINE_STATIC_NAME_STR(rightDoor, TXT("right door"));
DEFINE_STATIC_NAME_STR(exitDoor, TXT("exit door"));
DEFINE_STATIC_NAME_STR(availableSpaceCentre, TXT("available space centre")); // to know where it is placed in the world
DEFINE_STATIC_NAME_STR(availableSpaceCorner, TXT("available space corner")); // to know the actual size of the balcony

//

void TowersPrepareVariablesContext::process()
{
	an_assert(variables || wmpOwner);
	an_assert(roomGenerationInfo);

	halfTower = Utils::get_value_from(NAME(towersForceHalf), false, variables, wmpOwner);
	maxSize = Utils::get_value_from(NAME(towersMaxSize), Vector2::zero, variables, wmpOwner);
	maxTileSize = Utils::get_value_from(NAME(towersMaxTileSize), Vector2::zero, variables, wmpOwner);

	tileSize = roomGenerationInfo->get_tile_size();

	// auto params
	if (!maxTileSize.is_zero())
	{
		if (!maxSize.is_zero())
		{
			maxSize.x = min(maxSize.x, max(1.0f, maxTileSize.x) * tileSize);
			maxSize.y = min(maxSize.y, max(1.0f, maxTileSize.y) * tileSize);
		}
		else
		{
			maxSize.x = max(1.0f, maxTileSize.x) * tileSize;
			maxSize.y = max(1.0f, maxTileSize.y) * tileSize;
		}
	}

	if (maxSize.is_zero())
	{
		maxSize = roomGenerationInfo->get_play_area_rect_size();
	}

	// keep it at least 3 tiles each size (to have round tower)
	maxSize.x = max(maxSize.x, tileSize * 3.0f);
	maxSize.y = max(maxSize.y, tileSize * 3.0f);

	// keep it inside
	maxSize.x = min(maxSize.x, roomGenerationInfo->get_play_area_rect_size().x);
	maxSize.y = min(maxSize.y, roomGenerationInfo->get_play_area_rect_size().y);

	// if set here, means it is forced
	if (!halfTower)
	{
		halfTower = false;
		if (roomGenerationInfo->is_small())
		{
			maxSize = roomGenerationInfo->get_play_area_rect_size();
			halfTower = true;
		}
		else if (maxSize.x < tileSize * 3.0f - 0.02f ||
			maxSize.y < tileSize * 3.0f - 0.02f)
		{
			maxSize.x = tileSize * 3.0f;
			maxSize.y = tileSize * 2.0f;
			halfTower = true;
		}
	}

	towerDiameter = halfTower ? min(maxSize.x, (maxSize.y - tileSize * 0.5f) * 2.0f) : min(maxSize.x, maxSize.y);
	towerRadius = towerDiameter * 0.5f;
	innerTowerRadius = towerRadius - tileSize; // to leave space for catwalk}
}

//

REGISTER_FOR_FAST_CAST(TowersInfo);

TowersInfo::TowersInfo()
{
}

TowersInfo::~TowersInfo()
{
}

bool TowersInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= mainMeshGenerator.load_from_xml(_node, TXT("mainMeshGenerator"));
	result &= towerMeshGenerator.load_from_xml(_node, TXT("towerMeshGenerator"));

	_lc.load_group_into(mainMeshGenerator);
	_lc.load_group_into(towerMeshGenerator);

	for_every(node, _node->children_named(TXT("environmentAnchor")))
	{
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("variableName"), setAnchorVariable);
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("setVariableName"), setAnchorVariable);
		getAnchorPOI = node->get_name_attribute_or_from_child(TXT("getFromPOI"), getAnchorPOI);
		roomCentrePOI = node->get_name_attribute_or_from_child(TXT("roomCentrePOI"), roomCentrePOI);
	}

	for_every(regionPieceNode, _node->children_named(TXT("asRegionPiece")))
	{
		result &= towersCount.load_from_xml(regionPieceNode, TXT("towersCount"));
		result &= towersBetweenDoorsCount.load_from_xml(regionPieceNode, TXT("towersBetweenDoorsCount"));

		Framework::RegionGenerator::LoadingContext lc(_lc);
		for_every(node, regionPieceNode->children_named(TXT("connector")))
		{
			PieceCombiner::Connector<Framework::RegionGenerator> connector;
			if (connector.load_from_xml(node, &lc))
			{
				connector.be_normal_connector();
				connectors.push_back(connector);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load a connector"));
				result = false;
			}
		}

		for_every(node, regionPieceNode->children_named(TXT("towerConnectors")))
		{
			towerConnectorsPriorityStartsAt = node->get_float_attribute_or_from_child(TXT("priorityStartsAt"), towerConnectorsPriorityStartsAt);
			if (auto* n = node->first_child_named(TXT("a")))
			{
				result &= towerConnectorA.load_from_xml(n, &lc);
			}
			if (auto* n = node->first_child_named(TXT("b")))
			{
				result &= towerConnectorB.load_from_xml(n, &lc);
			}
		}

		towerConnectorA.be_normal_connector();
		towerConnectorB.be_normal_connector();
	}

	return result;
}

bool TowersInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	Framework::RegionGenerator::PreparingContext pc(_library);
	for_every(connector, connectors)
	{
		result &= connector->prepare(nullptr, &pc);
	}
	result &= towerConnectorA.prepare(nullptr, &pc);
	result &= towerConnectorB.prepare(nullptr, &pc);

	return result;
}

bool TowersInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr TowersInfo::create_copy() const
{
	TowersInfo * copy = new TowersInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

RefCountObjectPtr<PieceCombiner::Piece<Framework::RegionGenerator>> TowersInfo::generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Framework::Region* _region, REF_ Random::Generator & _randomGenerator) const
{
	RefCountObjectPtr<PieceCombiner::Piece<Framework::RegionGenerator>> ptr;

	ptr = new PieceCombiner::Piece<Framework::RegionGenerator>();

	for_every(connector, connectors)
	{
		ptr->add_connector(*connector);
	}

	int useCount = towersCount.get(_randomGenerator);
	useCount = max(useCount, _generator.get_outer_connector_count()); // at least enough towers to have door on each one, use only if using randomised count
	if (_region)
	{
		useCount = _region->get_variables().get_value<int>(NAME(towersCount), useCount); // force using what we're provided with
	}
	// for open world, by this point we should already have tower count properly set
	if (useCount == 0)
	{
		int inbetween = towersBetweenDoorsCount.get(_randomGenerator);
		if (_region)
		{
			inbetween = _region->get_variables().get_value<int>(NAME(towersBetweenDoorsCount), inbetween);
		}
		inbetween = max(0, inbetween);
		if (_generator.get_outer_connector_count() >= 3) inbetween = min(inbetween, 1);
		if (_generator.get_outer_connector_count() >= 5) inbetween = 0;
		useCount = _generator.get_outer_connector_count() * (1 + inbetween);
	}

	// arrange connectors that will change into doors later
	for_count(int, i, useCount)
	{
		int ni = (i + 1) % useCount;
		PieceCombiner::Connector<Framework::RegionGenerator> aConnector = towerConnectorA;
		PieceCombiner::Connector<Framework::RegionGenerator> bConnector = towerConnectorB;

		/*
			based on prototype

				group by priority to have same piece connector group handled nicely
				<connector name="0*-1" priority="1000" samePieceConnectorGroup="0-1" doorInRoomTags="at=0 to=1" doorType="testVR:door" plug="ring" canCreateNewPieces="yes"/>
				<connector name="0-*1" priority="1000" samePieceConnectorGroup="0-1" doorInRoomTags="at=1 to=0" doorType="testVR:door" socket="ring" canCreateNewPieces="no"/>
		 */

		aConnector.priority = towerConnectorsPriorityStartsAt + (float)i;
		bConnector.priority = towerConnectorsPriorityStartsAt + (float)i;

		aConnector.name = Name(String::printf(TXT("%i*-%i"), i, ni));
		bConnector.name = Name(String::printf(TXT("%i-*%i"), i, ni));
		aConnector.samePieceConnectorGroup = Name(String::printf(TXT("%i-%i"), i, ni));
		bConnector.samePieceConnectorGroup = aConnector.samePieceConnectorGroup;

		aConnector.def.doorInRoomTags.set_tag(NAME(at), i);
		aConnector.def.doorInRoomTags.set_tag(NAME(to), ni);
		aConnector.def.doorInRoomTags.set_tag(NAME(atToDir), 1);
		bConnector.def.doorInRoomTags.set_tag(NAME(to), i);
		bConnector.def.doorInRoomTags.set_tag(NAME(at), ni);
		bConnector.def.doorInRoomTags.set_tag(NAME(atToDir), -1);

		ptr->add_connector(aConnector);
		ptr->add_connector(bConnector);
	}

	return ptr;
}

bool TowersInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	Towers ts(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("towers \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= ts.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

Towers::Towers(TowersInfo const * _info)
: info(_info)
{
}

Towers::~Towers()
{
}

bool Towers::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;
	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();
	Random::Generator randomGenerator = _room->get_individual_random_generator();
	randomGenerator.advance_seed(389780, 977934);

#ifdef OUTPUT_GENERATION
	output(TXT("generating towers %S"), randomGenerator.get_seed_string().to_char());
#endif

	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("towers"))));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::purple));
	dv->activate();
#endif

	auto* mainMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->mainMeshGenerator.get(variables));
	if (!mainMeshGenerator)
	{
		error(TXT("no main mesh generator provided"));
		an_assert(false);
		return false;
	}

	auto* towerMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->towerMeshGenerator.get(variables));
	if (!towerMeshGenerator)
	{
		error(TXT("no tower mesh generator provided"));
		an_assert(false);
		return false;
	}

	// get count basing on doors
	int towerCount = 0;
	for_every_ptr(dir, _room->get_all_doors())
	{
		if (dir->get_tags().has_tag(NAME(at)))
		{
			int atTower = dir->get_tags().get_tag_as_int(NAME(at));
			towerCount = max(towerCount, atTower + 1);
		}
		if (dir->get_tags().has_tag(NAME(to)))
		{
			int toTower = dir->get_tags().get_tag_as_int(NAME(to));
			towerCount = max(towerCount, toTower + 1);
		}
	}

	{
		// xTowerDoorIdx is used to connect to xTowers - indices are the same in both, in one it is door in room's index, in other it is tower index
		struct TowerDoor
		{
			int tower;
			int doorIdx;
			Name connectorTag;
			TowerDoor() {}
			TowerDoor(int _tower, int _doorIdx, Name const & _connectorTag) : tower(_tower), doorIdx(_doorIdx), connectorTag(_connectorTag) {}

			static bool does_contain_tower(Array<TowerDoor> const& _array, int _tower)
			{
				for_every(t, _array)
				{
					if (t->tower == _tower)
					{
						return true;
					}
				}
				return false;
			}

			static int get_index_by_tower(Array<TowerDoor> const& _array, int _tower)
			{
				for_every(t, _array)
				{
					if (t->tower == _tower)
					{
						return for_everys_index(t);
					}
				}
				return NONE;
			}
		};
		ARRAY_PREALLOC_SIZE(TowerDoor, inTowerDoors, _room->get_all_doors().get_size());
		ARRAY_PREALLOC_SIZE(TowerDoor, outTowerDoors, _room->get_all_doors().get_size());

		bool alignToTiles = variables.get_value(NAME(alignToTiles), false);

		{
			ARRAY_PREALLOC_SIZE(TowerDoor, anyTowerDoors, _room->get_all_doors().get_size());

			for_every_ptr(dir, _room->get_all_doors())
			{
				if (dir->get_tags().has_tag(NAME(any)))
				{
					anyTowerDoors.push_back(TowerDoor(NONE, for_everys_index(dir), dir->get_connector_tag()));
				}
				if (dir->get_tags().has_tag(NAME(in)))
				{
					inTowerDoors.push_back(TowerDoor(NONE, for_everys_index(dir), dir->get_connector_tag()));
				}
				if (dir->get_tags().has_tag(NAME(out)))
				{
					outTowerDoors.push_back(TowerDoor(NONE, for_everys_index(dir), dir->get_connector_tag()));
				}
			}

			if (inTowerDoors.is_empty() && outTowerDoors.is_empty())
			{
				if (!anyTowerDoors.is_empty())
				{
					// we add towers counter clockwise, reverse the order here
					// if they have connector tags, we will be using those to place them properly
					for_every_reverse(i, anyTowerDoors)
					{
						inTowerDoors.push_back(*i);
					}
				}
				else
				{
					error(TXT("no doors with door tags \"in\", \"out\", \"any\""));
				}
			}
		}
		{
			int const inTowerCount = inTowerDoors.get_size();
			int const outTowerCount = outTowerDoors.get_size();
			int const inOutTowerCount = inTowerCount + outTowerCount;
			if (inOutTowerCount > towerCount)
			{
				error(TXT("not enough towers, how is that possible?"));
				an_assert(false);
				return false;
			}

			bool placedTowers = false;

			if (!placedTowers)
			{
				ARRAY_PREALLOC_SIZE(Name, doorConnectorTags, towerCount);
				{
					for_every(td, inTowerDoors)
					{
						if (td->connectorTag.is_valid())
						{
							doorConnectorTags.push_back(td->connectorTag);
						}
					}
					for_every(td, outTowerDoors)
					{
						if (td->connectorTag.is_valid())
						{
							doorConnectorTags.push_back(td->connectorTag);
						}
					}
				}
				if (!doorConnectorTags.is_empty())
				{
					if (PilgrimageInstanceOpenWorld::get())
					{
						Optional<DirFourClockwise::Type> preferredDir;
						ARRAY_PREALLOC_SIZE(Name, atTowerConnectorTags, towerCount);
						ARRAY_PREALLOC_SIZE(DirFourClockwise::Type, atTowerDirs, towerCount);
						PilgrimageInstanceOpenWorld::distribute_connector_tags_evenly(doorConnectorTags, OUT_ atTowerConnectorTags, OUT_ atTowerDirs,
							true /*looped*/, NP /* no breaker */, preferredDir, towerCount, PilgrimageInstanceOpenWorldConnectorTagDistribution::CounterClockwise,
							_room->get_individual_random_generator());
						for_every(atct, atTowerConnectorTags)
						{
							if (atct->is_valid())
							{
								bool placed = false;
								if (!placed)
								{
									for_every(td, inTowerDoors)
									{
										if (td->connectorTag == *atct &&
											td->tower == NONE)
										{
											td->tower = for_everys_index(atct);
											placed = true;
											break;
										}
									}
								}
								if (!placed)
								{
									for_every(td, outTowerDoors)
									{
										if (td->connectorTag == *atct &&
											td->tower == NONE)
										{
											td->tower = for_everys_index(atct);
											placed = true;
											break;
										}
									}
								}
							}
						}
						placedTowers = true;
						for_every(td, inTowerDoors)
						{
							if (td->tower == NONE)
							{
								placedTowers = false;
								break;
							}
						}
						for_every(td, outTowerDoors)
						{
							if (td->tower == NONE)
							{
								placedTowers = false;
								break;
							}
						}
						if (!placedTowers)
						{
							error(TXT("could not placed all doors at towers"));
							AN_BREAK;
							return false;
						}
					}
					else
					{
						warn(TXT("no way to organise door tags, will revert to auto even distribution"))
					}
				}
			}

			if (!placedTowers)
			{
				// just place them evenly
				for_every(inTowerDoor, inTowerDoors)
				{
					inTowerDoor->tower = clamp((for_everys_index(inTowerDoor) * towerCount) / inOutTowerCount, 0, towerCount - 1);
				}
				for_every(outTowerDoor, outTowerDoors)
				{
					outTowerDoor->tower = clamp(((inTowerCount + for_everys_index(outTowerDoor)) * towerCount) / inOutTowerCount, 0, towerCount - 1);
				}
			}
		}

		for_every(inTowerDoor, inTowerDoors)
		{
			for_every(outTowerDoor, outTowerDoors)
			{
				if (outTowerDoor->tower == inTowerDoor->tower)
				{
					error(TXT("in and out tower should be different!"));
					AN_BREAK;
					return false;
				}
			}
		}

		//

		TowersPrepareVariablesContext tpvContext;
		tpvContext.use(roomGenerationInfo);
		tpvContext.use(variables);
		tpvContext.process();

		float tileSize = tpvContext.tileSize;
		
		//

		Framework::UsedLibraryStored<Framework::Mesh> mainMesh;
		// generate layout
		if (mainMeshGenerator)
		{
			SimpleVariableStorage mainVariables(variables);

			Random::Generator rg(randomGenerator);
			rg.advance_seed(97974, 979265);

			mainVariables.access<int>(NAME(towersCount)) = towerCount;

			Transform mainPlacement = Transform::identity;

			mainMesh = mainMeshGenerator->generate_temporary_library_mesh(::Framework::Library::get_current(),
				::Framework::MeshGeneratorRequest()
				.for_wmp_owner(_room)
				.no_lods()
				.using_parameters(mainVariables)
				.using_random_regenerator(rg)
				.using_mesh_nodes(_roomGeneratingContext.meshNodes));
			if (mainMesh.get())
			{
				_room->add_mesh(mainMesh.get(), mainPlacement);
				Framework::MeshNode::copy_not_dropped_to(mainMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes, mainPlacement);
			}
			else
			{
				error(TXT("could not generate main mesh"));
				result = false;
				AN_BREAK;
			}
		}
		else
		{
			error(TXT("no mesh generator for towers layout"));
			result = false;
			AN_BREAK;
		}

		if (mainMesh.get())
		{
			for_every_ref(meshNode, mainMesh->get_mesh_nodes()) // we will be referring to same mesh nodes and when we drop, the nodes in roomGeneratingContext will also be dropped
			{
				if (meshNode->name == NAME(tower))
				{
					int const* pIndex = meshNode->variables.get_existing<int>(NAME(index));
					if (pIndex)
					{
						SimpleVariableStorage towerVariables(variables);

						towerVariables.set_from(meshNode->variables);

						Transform towerPlacement = meshNode->placement;

						int towerIdx = *pIndex;

						Framework::UsedLibraryStored<Framework::Mesh> towerMesh;

						// add a tower to the room
						if (towerMeshGenerator)
						{
#ifdef OUTPUT_GENERATION_VARIABLES
							LogInfoContext log;
							log.log(TXT("generating mesh using:"));
							{
								LOG_INDENT(log);
								towerVariables.log(log);
							}
							log.output_to_output();
#endif

							Random::Generator useRG = randomGenerator.spawn();
							useRG = meshNode->variables.get_value<Random::Generator>(NAME(randomSeed), useRG);

							result = true;
							auto generatedMesh = towerMeshGenerator->generate_temporary_library_mesh(::Framework::Library::get_current(),
								::Framework::MeshGeneratorRequest()
								.for_wmp_owner(_room)
								.no_lods()
								.using_parameters(towerVariables)
								.using_random_regenerator(useRG)
								.using_mesh_nodes(_roomGeneratingContext.meshNodes));
							if (generatedMesh.get())
							{
								towerMesh = generatedMesh;

								_room->add_mesh(generatedMesh.get(), towerPlacement);
								Framework::MeshNode::copy_not_dropped_to(generatedMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes);
							}
							else
							{
								error(TXT("could not generate tower mesh \"%S\""), towerMeshGenerator->get_name().to_string().to_char());
								result = false;
							}
						}
						else
						{
							error(TXT("could not generate tower mesh \"%S\""), towerMeshGenerator->get_name().to_string().to_char());
							return false;
						}

						Transform leftDoorLocalPlacement = Transform::identity;
						Transform rightDoorLocalPlacement = Transform::identity;
						Transform exitDoorLocalPlacement = Transform::identity;
						Transform availableSpaceCentreLocalPlacement = Transform::identity;
						{
							if (auto* meshNode = towerMesh->access_mesh_node(NAME(leftDoor)))
							{
								leftDoorLocalPlacement = meshNode->placement;
								meshNode->drop();
							}
							else
							{
								error(TXT("no \"left door\" for tower"));
								result = false;
								AN_BREAK;
							}
							if (auto* meshNode = towerMesh->access_mesh_node(NAME(rightDoor)))
							{
								rightDoorLocalPlacement = meshNode->placement;
								meshNode->drop();
							}
							else
							{
								error(TXT("no \"right door\" for tower"));
								result = false;
								AN_BREAK;
							}
							if (auto* meshNode = towerMesh->access_mesh_node(NAME(exitDoor)))
							{
								exitDoorLocalPlacement = meshNode->placement;
								meshNode->drop();
							}
							else
							{
								error(TXT("no \"exit door\" for tower"));
								result = false;
								AN_BREAK;
							}
							if (auto* meshNode = towerMesh->find_mesh_node(NAME(availableSpaceCentre)))
							{
								availableSpaceCentreLocalPlacement = meshNode->placement;
							}
							else
							{
								error(TXT("no \"available space centre\" for tower"));
								result = false;
								AN_BREAK;
							}
						}

						// we want to fit available space into our playable vr space
						Transform availableSpaceCentreVRSpacePlacement = Transform::identity;
						{	// orient and move balcony in vr space
							Range2 vrSpaceTaken = Range2::empty; // relative to centre
							{	// prepare vr space taken
								// get space taken, first get doors to include whole doors and space beyond them
								vrSpaceTaken.include(RoomGenerationInfo::get_door_required_space(availableSpaceCentreLocalPlacement.to_local(leftDoorLocalPlacement), tileSize));
								vrSpaceTaken.include(RoomGenerationInfo::get_door_required_space(availableSpaceCentreLocalPlacement.to_local(rightDoorLocalPlacement), tileSize));
								// include corners
								for_every_ref(meshNode, towerMesh->get_mesh_nodes())
								{
									if (meshNode->name == NAME(availableSpaceCorner))
									{
										vrSpaceTaken.include(availableSpaceCentreLocalPlacement.location_to_local(meshNode->placement.get_translation()).to_vector2());
									}
								}
							}
							availableSpaceCentreVRSpacePlacement = RoomGeneratorUtils::get_placement_for_required_vr_space(_room, vrSpaceTaken, roomGenerationInfo, randomGenerator, alignToTiles);
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
							{
								Range2 vrSpaceTakenMoved = vrSpaceTaken.moved_by(availableSpaceCentreVRSpacePlacement.get_translation().to_vector2());
								Framework::DebugVisualiserUtils::add_range2_to_debug_visualiser(dv, vrSpaceTakenMoved, Colour::yellow);
								roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get());
							}
#endif
						}

						Transform leftDoorVRPlacement = availableSpaceCentreVRSpacePlacement.to_world(availableSpaceCentreLocalPlacement.to_local(leftDoorLocalPlacement));
						Transform rightDoorVRPlacement = availableSpaceCentreVRSpacePlacement.to_world(availableSpaceCentreLocalPlacement.to_local(rightDoorLocalPlacement));
						Transform exitDoorVRPlacement = availableSpaceCentreVRSpacePlacement.to_world(availableSpaceCentreLocalPlacement.to_local(exitDoorLocalPlacement));

						// place doors on this tower
						for_every(pDir, _room->get_all_doors())
						{
							auto* dir = *pDir;
							// place side doors that connect to other towers via passages
							if (dir->get_tags().has_tag(NAME(at)))
							{
								int atTower = dir->get_tags().get_tag_as_int(NAME(at));
								if (atTower == towerIdx)
								{
									int toTower = dir->get_tags().get_tag_as_int(NAME(to));
									bool goRight = mod(toTower - atTower, towerCount) == 1;
									if (dir->get_tags().has_tag(NAME(atToDir)))
									{
										goRight = dir->get_tags().get_tag_as_int(NAME(atToDir)) > 0;
									}
									Transform dirInTowerSpace = goRight? rightDoorLocalPlacement : leftDoorLocalPlacement;
									Transform dirVRPlacement = goRight ? rightDoorVRPlacement : leftDoorVRPlacement;
									Transform dirRoomPlacement = towerPlacement.to_world(dirInTowerSpace);
									if (!dir->is_vr_space_placement_set() ||
										!Framework::DoorInRoom::same_with_orientation_for_vr(dir->get_vr_space_placement(), dirVRPlacement))
									{
										if (dir->is_vr_placement_immovable())
										{
											dir = dir->grow_into_vr_corridor(NP, dirVRPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
										}
										dir->set_vr_space_placement(dirVRPlacement);
									}
									dir->set_placement(dirRoomPlacement);
									dir->set_group_id(towerIdx);
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
									{
										float doorWidth = dir->get_door()->calculate_vr_width();
										Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, dir, dir->get_vr_space_placement().get_translation().to_vector2(), dir->get_vr_space_placement().get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::red);
										Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, dir, dirVRPlacement.get_translation().to_vector2(), dirVRPlacement.get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::blue);
									}
#endif
#ifdef AN_DEVELOPMENT
									float doorWidth = dir->get_door()->calculate_vr_width();
									if (doorWidth == 0.0f)
									{
										doorWidth = roomGenerationInfo.get_tile_size();
									}
									an_assert(roomGenerationInfo.get_play_area_zone().does_contain(dir->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f));
#endif
								}
							}
							// check if this is one of in/out doors that belong to this tower
							bool placeDoorHere = false;
							{
								int inTowerDoorIdx = TowerDoor::get_index_by_tower(inTowerDoors, towerIdx);
								if (inTowerDoorIdx != NONE &&
									inTowerDoors[inTowerDoorIdx].doorIdx == for_everys_index(pDir))
								{
									placeDoorHere = true;
								}
								int outTowerDoorIdx = TowerDoor::get_index_by_tower(outTowerDoors, towerIdx);
								if (outTowerDoorIdx != NONE &&
									outTowerDoors[outTowerDoorIdx].doorIdx == for_everys_index(pDir))
								{
									placeDoorHere = true;
								}
							}
							if (placeDoorHere)
							{
								Transform dirInTowerSpace = exitDoorLocalPlacement;
								Transform dirVRPlacement = exitDoorVRPlacement;
								Transform dirRoomPlacement = towerPlacement.to_world(dirInTowerSpace);
								if (!dir->is_vr_space_placement_set() ||
									!Framework::DoorInRoom::same_with_orientation_for_vr(dir->get_vr_space_placement(), dirVRPlacement))
								{
									if (dir->is_vr_placement_immovable())
									{
										dir = dir->grow_into_vr_corridor(NP, dirVRPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
									}
									dir->set_vr_space_placement(dirVRPlacement);
								}
								dir->set_placement(dirRoomPlacement);
								dir->set_group_id(towerIdx);
#ifdef AN_DEVELOPMENT
								float doorWidth = dir->get_door()->calculate_vr_width();
								if (doorWidth == 0.0f)
								{
									doorWidth = roomGenerationInfo.get_tile_size();
								}
								an_assert(roomGenerationInfo.get_play_area_zone().does_contain(dir->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f));
#endif
							}
						}
					}
				}
			}
		}

#ifdef AN_DEVELOPMENT
		for_every_ptr(dir, _room->get_all_doors())
		{
			an_assert(dir->is_placed(), TXT("dir %i %S not placed"), for_everys_index(dir), dir->get_tags().to_string(true).to_char());
		}
#endif

		if (info->setAnchorVariable.is_valid())
		{
			Transform anchorProvided = Transform::identity;
			if (info->getAnchorPOI.is_valid())
			{
				_room->for_every_point_of_interest(info->getAnchorPOI, [&anchorProvided](Framework::PointOfInterestInstance* _poi)
					{
						anchorProvided = _poi->calculate_placement();
					});
			}

			Transform roomCentrePOI = anchorProvided;
			if (info->roomCentrePOI.is_valid())
			{
				_room->for_every_point_of_interest(info->roomCentrePOI, [&roomCentrePOI](Framework::PointOfInterestInstance* _poi)
					{
						roomCentrePOI = _poi->calculate_placement();
					});
			}

			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				piow->tag_open_world_directions_for_cell(_room);
				DirFourClockwise::Type dirs[] = { DirFourClockwise::Up
												, DirFourClockwise::Down
												, DirFourClockwise::Right
												, DirFourClockwise::Left };
				for_count(int, i, DirFourClockwise::NUM)
				{
					DirFourClockwise::Type inDir = dirs[i];
					if (auto* dir = piow->find_best_door_in_dir(_room, inDir))
					{
						Vector3 placementInDir = dir->get_hole_centre_WS();
						Vector3 actualInDir = placementInDir - roomCentrePOI.get_translation();
						actualInDir.z = 0.0f;
						actualInDir.normalise();

						// take inDir into account
						actualInDir = actualInDir.rotated_by_yaw(-(float)inDir * 90.0f);

						// where room centre should be
						Transform roomCentrePOIAsShouldBe = look_matrix(roomCentrePOI.get_translation(), actualInDir, Vector3::zAxis).to_transform();

						Transform inDirInRoom = roomCentrePOIAsShouldBe.to_world(roomCentrePOI.to_local(anchorProvided));

						_room->access_variables().access<Transform>(info->setAnchorVariable) = inDirInRoom;
						break;
					}
				}
			}
		}
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated towers"));
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
