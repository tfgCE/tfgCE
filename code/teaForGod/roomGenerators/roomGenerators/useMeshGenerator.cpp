#include "useMeshGenerator.h"

#include "..\roomGenerationInfo.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\environmentInfo.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_VARIABLES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

#define ACCUMULATE_TURN_SCORE

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

// door/room tags
DEFINE_STATIC_NAME(inaccessible);
DEFINE_STATIC_NAME(vrInaccessible);
DEFINE_STATIC_NAME(vrPlacementIgnorable);

// poi params
DEFINE_STATIC_NAME(forDoorTagged);
DEFINE_STATIC_NAME(vrAnchorId);
DEFINE_STATIC_NAME(groupId);

//

REGISTER_FOR_FAST_CAST(UseMeshGeneratorInfo);

UseMeshGeneratorInfo::UseMeshGeneratorInfo()
{
}

UseMeshGeneratorInfo::~UseMeshGeneratorInfo()
{
}

bool UseMeshGeneratorInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= meshGenerator.load_from_xml(_node, TXT("meshGenerator"));
	_lc.load_group_into(meshGenerator);

	atPOI = _node->get_name_attribute_or_from_child(TXT("atPOI"), atPOI);

	vrAnchorPOI = _node->get_name_attribute_or_from_child(TXT("vrAnchorPOI"), vrAnchorPOI);
	doorPOI = _node->get_name_attribute_or_from_child(TXT("doorPOI"), doorPOI);
	doorPOI = _node->get_name_attribute_or_from_child(TXT("placeDoorOnPOI"), doorPOI);
	placeDoorsTagged.load_from_xml_attribute_or_child_node(_node, TXT("placeDoorsTagged"));
	setVRPlacementForDoorsTagged.load_from_xml_attribute_or_child_node(_node, TXT("setVRPlacementForDoorsTagged"));

	worldSeparatorDoor = _node->get_bool_attribute_or_from_child_presence(TXT("worldSeparatorDoor"), worldSeparatorDoor);

	allowVRAnchorTurn180 = _node->get_bool_attribute_or_from_child_presence(TXT("allowVRAnchorTurn180"), allowVRAnchorTurn180);
	allowVRAnchorTurn180 = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowVRAnchorTurn180"), ! allowVRAnchorTurn180);

	error_loading_xml_on_assert(!(vrAnchorPOI.is_valid() && ! doorPOI.is_valid()), result, _node, TXT("if vrAnchorPOI is set, doorPOI should be set too"));

	return result;
}

bool UseMeshGeneratorInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool UseMeshGeneratorInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	if (meshGenerator.is_value_set())
	{
		auto n = meshGenerator.get_value();
		_renamer.apply_to(n);
		meshGenerator.set(n);
	}

	return result;
}

Framework::RoomGeneratorInfoPtr UseMeshGeneratorInfo::create_copy() const
{
	UseMeshGeneratorInfo * copy = new UseMeshGeneratorInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool UseMeshGeneratorInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	UseMeshGenerator sr(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("mesh generator (room) \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= sr.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

UseMeshGenerator::UseMeshGenerator(UseMeshGeneratorInfo const * _info)
: info(_info)
{
}

UseMeshGenerator::~UseMeshGenerator()
{
}

bool UseMeshGenerator::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;
	auto roomGenerationInfo = RoomGenerationInfo::get();

#ifdef OUTPUT_GENERATION
	output(TXT("generating room using mesh generator %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif

	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);

	bool allDoorsHaveVRPlacementSet = true;
	bool someDoorsHaveVRPlacementSet = false;
	bool ignoreVRPlacement = _room->get_tags().get_tag(NAME(inaccessible)) || _room->get_tags().get_tag(NAME(vrInaccessible)) || _room->get_tags().get_tag(NAME(vrPlacementIgnorable));

	for_every_ptr(door, _room->get_all_doors())
	{
		if (door->may_ignore_vr_placement())
		{
			continue;
		}
		if (door->is_vr_space_placement_set())
		{
			someDoorsHaveVRPlacementSet = true;
		}
		else
		{
			allDoorsHaveVRPlacementSet = false;
		}
		if (info->worldSeparatorDoor)
		{
			if (auto* d = door->get_door())
			{
				d->be_world_separator_door(NP, _room);
			}
		}
	}

	Framework::UsedLibraryStored<Framework::MeshGenerator> infoMeshGenerator;
	infoMeshGenerator.set_name(info->meshGenerator.get(_room, Framework::LibraryName::invalid()));
	infoMeshGenerator.find(Framework::Library::get_current());
	
	if (!info->doorPOI.is_valid() && !allDoorsHaveVRPlacementSet && !ignoreVRPlacement)
	{
		error(TXT("missing doorPOI, requires all door to have vr placement set - do not use this generator if there are doors without vr placement set! we may move door but we won't set vr placement - you may want to use tags for room: inaccessible, vrInaccessible or vrPlacementIgnorable"));
		return false;
	}
	else
	{
		struct POIInfo
		{
			Transform placement;
			TagCondition forDoorTagged; // use connector's doorInRoomTags to tag the door properly (for door we should use one door only, for vr anchor we may use it to specify multiple doors)
			Name vrAnchorId; // has to be set for vr anchor and door POIs, makes it easier to find groups
			int groupId = -1; // for navigation
			Optional<int> doorInRoomIdx; // might be used for doors to tell their index

			POIInfo() {}
			explicit POIInfo(Framework::PointOfInterestInstance* _poi)
			{
				placement = _poi->calculate_placement();
				if (auto* param = _poi->access_parameters().get_existing<TagCondition>(NAME(forDoorTagged)))
				{
					forDoorTagged = *param;
				}
				if (auto* param = _poi->access_parameters().get_existing<Name>(NAME(vrAnchorId)))
				{
					vrAnchorId = *param;
				}
				if (auto* param = _poi->access_parameters().get_existing<int>(NAME(groupId)))
				{
					groupId = *param;
				}
			}
		};
		struct IndexedDoorInRoom
		{
			Framework::DoorInRoom* door = nullptr;
			int doorInRoomIdx; // in room

			IndexedDoorInRoom() {}
			explicit IndexedDoorInRoom(Framework::DoorInRoom* _door, int _doorInRoomIdx)
			: door(_door)
			, doorInRoomIdx(_doorInRoomIdx)
			{}
		};

		if (auto * mg = infoMeshGenerator.get())
		{
#ifdef OUTPUT_GENERATION_VARIABLES
			LogInfoContext log;
			log.log(TXT("generating mesh using:"));
			{
				LOG_INDENT(log);
				variables.log(log);
			}
			log.output_to_output();
#endif
			result = true;
			auto generatedMesh = mg->generate_temporary_library_mesh(::Framework::Library::get_current(),
				::Framework::MeshGeneratorRequest()
					.for_wmp_owner(_room)
					.no_lods()
					.using_parameters(variables)
					.using_random_regenerator(_room->get_individual_random_generator())
					.using_mesh_nodes(_roomGeneratingContext.meshNodes));
			if (generatedMesh.get())
			{
				Transform placement = Transform::identity;
				if (info->atPOI.is_valid())
				{
					_room->for_every_point_of_interest(info->atPOI, [&placement](Framework::PointOfInterestInstance* _poi) { placement = _poi->calculate_placement(); });
				}
				_room->add_mesh(generatedMesh.get(), placement);
				_room->place_pending_doors_on_pois();
				Framework::MeshNode::copy_not_dropped_to(generatedMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes);
				if (info->vrAnchorPOI.is_valid() && info->doorPOI.is_valid())
				{
					Array<POIInfo> vrAnchorPOIs;
					_room->for_every_point_of_interest(info->vrAnchorPOI, [&vrAnchorPOIs](Framework::PointOfInterestInstance* _poi) { vrAnchorPOIs.push_back(POIInfo(_poi)); });
					if (!vrAnchorPOIs.is_empty())
					{
						Array<POIInfo> doorPOIs;
						_room->for_every_point_of_interest(info->doorPOI, [&doorPOIs](Framework::PointOfInterestInstance* _poi) { doorPOIs.push_back(POIInfo(_poi)); });
						// place doors on POIs
						{
							Array<IndexedDoorInRoom> doorsToPlace;
							doorsToPlace.make_space_for(_room->get_all_doors().get_size());
							for_every_ptr(door, _room->get_all_doors())
							{
								if (info->placeDoorsTagged.check(door->get_tags()))
								{
									doorsToPlace.push_back(IndexedDoorInRoom(door, for_everys_index(door)));
								}
							}
							// place doors and bind doorInRoom index to POI, one of doorPOIs may be attached to one door only!
							if (doorPOIs.get_size() >= doorsToPlace.get_size())
							{
								for_every(doorToPlace, doorsToPlace)
								{
									auto* door = doorToPlace->door;
									Optional<int> foundDoorPOI;
									if (!foundDoorPOI.is_set())
									{
										for_every(doorPOI, doorPOIs)
										{
											if (!doorPOI->doorInRoomIdx.is_set() &&
												!doorPOI->forDoorTagged.is_empty() &&
												doorPOI->forDoorTagged.check(door->get_tags()))
											{
												foundDoorPOI = for_everys_index(doorPOI);
											}
										}
									}
									if (!foundDoorPOI.is_set())
									{
										for_every(doorPOI, doorPOIs)
										{
											if (!doorPOI->doorInRoomIdx.is_set() &&
												doorPOI->forDoorTagged.is_empty())
											{
												foundDoorPOI = for_everys_index(doorPOI);
											}
										}
									}
									if (foundDoorPOI.is_set())
									{
										auto& useDoorPOI = doorPOIs[foundDoorPOI.get()];
										useDoorPOI.doorInRoomIdx = doorToPlace->doorInRoomIdx;
										Transform doorPlacement = useDoorPOI.placement;
										door->set_placement(doorPlacement);
									}
									else
									{
										error(TXT("couldn't match door POI for door, \"%S\" provided by mesh generator (provided:%i, required at least:%i) maybe limit doors with placeDoorsTagged"), info->doorPOI.to_char(), doorPOIs.get_size(), doorsToPlace.get_size());
									}
								}
							}
							else
							{
								error_dev_ignore(TXT("failed generating room %S : %S"), _room->get_name().to_char(), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("--"));
								for_every(dtp, doorsToPlace)
								{
									error_dev_ignore(TXT("%i. to r%p : %S"), dtp->doorInRoomIdx, dtp->door->get_room_on_other_side(), dtp->door->get_room_on_other_side() ? dtp->door->get_room_on_other_side()->get_name().to_char() : TXT("--"));
								}
								error(TXT("not enough door POIs named \"%S\" provided by mesh generator (provided:%i, required at least:%i) maybe limit doors with placeDoorsTagged"), info->doorPOI.to_char(), doorPOIs.get_size(), doorsToPlace.get_size());
								result = false;
							}
						}
						Array<IndexedDoorInRoom> doorsToSetVRPlacement;
						// get doors that require vr placement to be set
						{
							doorsToSetVRPlacement.make_space_for(_room->get_all_doors().get_size());
							for_every_ptr(door, _room->get_all_doors())
							{
								if (info->setVRPlacementForDoorsTagged.check(door->get_tags()))
								{
									doorsToSetVRPlacement.push_back(IndexedDoorInRoom(door, for_everys_index(door)));
								}
							}
						}
						struct GetVRPlacementForDoor
						{
							static Optional<Transform> find(Array<POIInfo> const& vrAnchorPOIs, Array<POIInfo> const& doorPOIs, IndexedDoorInRoom const& doorToPlace)
							{
								auto* door = doorToPlace.door;
								Transform doorPlacement = door->get_placement();
								Transform vrPlacement = vrAnchorPOIs.get_first().placement.to_local(doorPlacement);
								bool found = false;
								if (!found)
								{
									// find by forDoorTagged
									for_every(vrap, vrAnchorPOIs)
									{
										if (!vrap->forDoorTagged.is_empty() &&
											vrap->forDoorTagged.check(door->get_tags()))
										{
											vrPlacement = vrap->placement.to_local(doorPlacement);
											found = true;
										}
									}
								}
								if (!found)
								{
									// find by vrAnchorId
									// (first find the right doorPOI, then matching vrAnchorPOI)
									for_every(doorPOI, doorPOIs)
									{
										if (doorPOI->doorInRoomIdx.is_set() &&
											doorToPlace.doorInRoomIdx == doorPOI->doorInRoomIdx.get() &&
											doorPOI->vrAnchorId.is_valid())
										{
											for_every(vrap, vrAnchorPOIs)
											{
												if (vrap->vrAnchorId == doorPOI->vrAnchorId)
												{
													vrPlacement = vrap->placement.to_local(doorPlacement);
													found = true;
												}
											}
										}
									}
								}
								if (!found)
								{
									// find closest
									float bestDistance = 100000.0f;
									for_every(vrap, vrAnchorPOIs)
									{
										if (vrap->forDoorTagged.is_empty())
										{
											float dist = (vrap->placement.get_translation() - doorPlacement.get_translation()).length();
											dist += 10.0f * abs(vrap->placement.get_translation().z - doorPlacement.get_translation().z); // additional penalty if anchor and door are on a different level
											if (bestDistance > dist)
											{
												bestDistance = dist;
												vrPlacement = vrap->placement.to_local(doorPlacement);
											}
										}
									}
								}
								if (found)
								{
									return vrPlacement;
								}
								else
								{
									return NP;
								}
							}
						};
						if (info->allowVRAnchorTurn180)
						{
							int fitsAsItIs = 0;
							int fitsTurned180 = 0;
							// check for each door that has already set vr placement if it fits better turned rather than not
							// if it is so and there are more of such doors, then do 180
							for_every(doorToPlace, doorsToSetVRPlacement)
							{
								auto* door = doorToPlace->door;
								if (door->get_tags().get_tag(NAME(inaccessible)) ||
									door->get_tags().get_tag(NAME(vrInaccessible)) ||
									door->get_tags().get_tag(NAME(vrPlacementIgnorable)))
								{
									// should ignore vr placement
									continue;
								}
								if (door->is_vr_space_placement_set())
								{
									Transform doorPlacement = door->get_placement();
									Transform defaultVRPlacement = vrAnchorPOIs.get_first().placement.to_local(doorPlacement);
									{
										Optional<Transform> foundVRPlacement = GetVRPlacementForDoor::find(vrAnchorPOIs, doorPOIs, *doorToPlace);
										if (foundVRPlacement.is_set())
										{
											defaultVRPlacement = foundVRPlacement.get();
										}
									}
									int scoreAsItIs = 0;
									int scoreTurned180 = 0;
									auto const & rgi = RoomGenerationInfo::get();
									float const tileSize = rgi.get_tile_size();
									for_count(int, turnedScenario, 2)
									{
										Transform vrPlacement = defaultVRPlacement;
										int& score = turnedScenario ? scoreTurned180 : scoreAsItIs;
										if (turnedScenario)
										{
											vrPlacement = defaultVRPlacement.to_world(Transform::do180);
										}
										Transform doorVRPlacement = door->get_vr_space_placement();
										if (Framework::DoorInRoom::same_with_orientation_for_vr(doorVRPlacement, vrPlacement))
										{
											score += 100;
										}
										else 
										{
											if (!door->is_vr_placement_immovable())
											{
												score += 20; // a bonus
											}

											float yaw = Rotator3::get_yaw(vrPlacement.get_axis(Axis::Forward));
											float doorYaw = Rotator3::get_yaw(doorVRPlacement.get_axis(Axis::Forward));
											float diff = Rotator3::normalise_axis(doorYaw - yaw);
											if (abs(diff) < 1.0f)
											{
												// same door axis and same dir
												// we score if:
												// + doorVRPlacement is on the same axis as vrPlacement and doorVRPlacement is in front of vrPlacement (outbound!)
												// + doorVRPlacement is in front of vrPlacement (outbound!) and there's at least one tile distance between two
												Vector3 doorRelLoc = vrPlacement.location_to_local(doorVRPlacement.get_translation());
												if (doorRelLoc.x < 0.03f && doorRelLoc.y > 0.0f)
												{
													score += 30;
												}
												else if (doorRelLoc.y >= tileSize)
												{
													score += 20;
												}
											}
											else if (abs(diff) < 85.0f)
											{
												Vector3 doorRelLoc = vrPlacement.location_to_local(doorVRPlacement.get_translation());
												if (doorRelLoc.y >= tileSize)
												{
													// any chance for a good turn?
													score += 10;
												}
											}
											else if (abs(diff) >= 85.0f)
											{
												// opposite directions
												// + if they lay outside each other's corridors (tile size) 
												// + if they lay in the same x halve
												Vector3 doorRelLoc = vrPlacement.location_to_local(doorVRPlacement.get_translation());
												Vector3 relLoc = doorVRPlacement.location_to_local(vrPlacement.get_translation());
												if (abs(doorRelLoc.x) >= tileSize * 0.5f &&
													abs(relLoc.x) >= tileSize * 0.5f)
												{
													score += 10;
													if (abs(diff) > 150.0f)
													{
														// u turn?
														score += 10;
													}
													else
													{
														// both are outbound, both point at where doors should be!
														// doorVRPlacement should be in front, but because they both point at required placement, relLoc should be in back
														if (doorRelLoc.y > 0.0f && relLoc.y < 0.0f)
														{
															// "90" corner
															score += 10;
														}
													}
												}
												else if (abs(diff) > 170.0f)
												{
													// elevator?
													score += 5;
												}
											}
										}
									}

#ifdef ACCUMULATE_TURN_SCORE
									fitsAsItIs += scoreAsItIs;
									fitsTurned180 += scoreTurned180;
#else
									if (scoreTurned180 > scoreAsItIs)
									{
										++fitsTurned180;
									}
									else
									{
										++fitsAsItIs;
									}
#endif
								}
							}

							if (fitsTurned180 > fitsAsItIs) // turn 180
							{
								for_every(vrap, vrAnchorPOIs)
								{
									vrap->placement = vrap->placement.to_world(Transform::do180);
								}
							}
						}
						// set vr placement for doors
						{
							for_every(doorToPlace, doorsToSetVRPlacement)
							{
								auto* door = doorToPlace->door;
								if (door->get_tags().get_tag(NAME(inaccessible)) ||
									door->get_tags().get_tag(NAME(vrInaccessible)) ||
									door->get_tags().get_tag(NAME(vrPlacementIgnorable)))
								{
									// should ignore vr placement
									continue;
								}
								Transform doorPlacement = door->get_placement();
								Transform vrPlacement = vrAnchorPOIs.get_first().placement.to_local(doorPlacement);
								int groupId = NONE;
								for_every(doorPOI, doorPOIs)
								{
									if (doorPOI->doorInRoomIdx.is_set() &&
										doorToPlace->doorInRoomIdx == doorPOI->doorInRoomIdx.get())
									{
										groupId = doorPOI->groupId;
									}
								}
								{
									Optional<Transform> foundVRPlacement = GetVRPlacementForDoor::find(vrAnchorPOIs, doorPOIs, *doorToPlace);
									if (foundVRPlacement.is_set())
									{
										vrPlacement = foundVRPlacement.get();
									}
								}
								if ((!door->is_vr_space_placement_set() ||
									!Framework::DoorInRoom::same_with_orientation_for_vr(door->get_vr_space_placement(), vrPlacement)))
								{
									if (door->is_vr_placement_immovable())
									{
										door = door->grow_into_vr_corridor(NP, vrPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
										door->set_placement(doorPlacement); // place new door where the old was
									}
									door->set_vr_space_placement(vrPlacement);
								}
								if (groupId != NONE)
								{
									door->set_group_id(groupId);
								}
							}
						}
					}
					else
					{
						error(TXT("no vr anchor POI named \"%S\" provided by mesh generator"), info->vrAnchorPOI.to_char());
						result = false;
					}
				}
				else if (info->doorPOI.is_valid()) // place doors where they should be
				{
					if (!ignoreVRPlacement)
					{
						error(TXT("if no vrAnchorPOI provided, this should not be used for rooms that ignore vr placement, if they should ignore vr placement, add tag \"inaccessible\", \"vrInaccessible\" OR \"vrPlacementIgnorable\""));
						result = false;
					}
					an_assert(ignoreVRPlacement);
					Array<Transform> doorPlacements;
					_room->for_every_point_of_interest(info->doorPOI, [&doorPlacements](Framework::PointOfInterestInstance* _poi) { doorPlacements.push_back(_poi->calculate_placement()); });
					ARRAY_PREALLOC_SIZE(Framework::DoorInRoom*, doorsToPlace, _room->get_all_doors().get_size());
					for_every_ptr(door, _room->get_all_doors())
					{
						if (info->placeDoorsTagged.check(door->get_tags()))
						{
							doorsToPlace.push_back(door);
						}
					}
					if (doorPlacements.get_size() >= doorsToPlace.get_size())
					{
						for_every_ptr(doorToPlace, doorsToPlace)
						{
							auto* door = doorToPlace;
							Transform doorPlacement = doorPlacements[for_everys_index(doorToPlace)];
							door->set_placement(doorPlacement);
						}
					}
					else
					{
						error_dev_ignore(TXT("failed generating room %S : %S"), _room->get_name().to_char(), _room->get_room_type() ? _room->get_room_type()->get_name().to_string().to_char() : TXT("--"));
						for_every_ptr(dtp, doorsToPlace)
						{
							error_dev_ignore(TXT("dr'%p to r%p : %S"), dtp, dtp->get_room_on_other_side(), dtp->get_room_on_other_side() ? dtp->get_room_on_other_side()->get_name().to_char() : TXT("--"));
						}
						error(TXT("not enough door POIs named \"%S\" provided by mesh generator (provided:%i, required at least:%i)"), info->doorPOI.to_char(), doorPlacements.get_size(), _room->get_all_doors().get_size());
						result = false;
					}
				}
			}
			else
			{
				error(TXT("could not generate mesh"));
				result = false;
			}
		}
		else
		{
			error(TXT("no mesh generator provided/found"));
			return false;
		}
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated room using mesh generator"));
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
