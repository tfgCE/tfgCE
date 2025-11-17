#include "gse_temporaryObject.h"

#include "..\..\appearance\controllers\ac_particlesUtils.h"
#include "..\..\game\game.h"
#include "..\..\game\poiManager.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleTemporaryObjects.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\object\temporaryObject.h"
#include "..\..\world\room.h"
#include "..\..\world\subWorld.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Elements::TemporaryObject::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	allowToFail = _node->get_bool_attribute_or_from_child_presence(TXT("allowToFail"), allowToFail);

	objectVar = _node->get_name_attribute_or_from_child(TXT("objectVar"), objectVar);
	instigatorObjectVar = _node->get_name_attribute_or_from_child(TXT("instigatorObjectVar"), instigatorObjectVar);
	start = _node->get_name_attribute_or_from_child(TXT("start"), start);
	stop = _node->get_bool_attribute_or_from_child_presence(TXT("stop"), stop);
	storeAsVar = _node->get_name_attribute_or_from_child(TXT("storeAsVar"), storeAsVar);
	storeAbsolutePlacementAsVar = _node->get_name_attribute_or_from_child(TXT("storeAbsolutePlacementAsVar"), storeAbsolutePlacementAsVar);

	Name stopVar;
	stopVar = _node->get_name_attribute_or_from_child(TXT("stopVar"), stopVar);
	if (stopVar.is_valid())
	{
		objectVar = stopVar;
		stop = true;
	}

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	result &= spawn.load_from_xml(_node, TXT("spawn"), _lc);

	systemTagRequired.load_from_xml_attribute_or_child_node(_node, TXT("systemTagRequired"));

	inRoomVar.load_from_xml(_node, TXT("inRoomVar"));
	poi.load_from_xml(_node, TXT("poi"));
	nearestPOI.load_from_xml(_node, TXT("nearestPOI"));
	nearestPOIAdditionalDistance.load_from_xml(_node, TXT("nearestPOIAdditionalDistance"));
	occupyPOI.load_from_xml(_node, TXT("occupyPOI"));
	occupyPOITime.load_from_xml(_node, TXT("occupyPOITime"));
	socket.load_from_xml(_node, TXT("socket"));
	socketVar.load_from_xml(_node, TXT("socketVar"));
	attached.load_from_xml(_node, TXT("attached"));
	relativePlacement.load_from_xml_child_node(_node, TXT("relativePlacement"));
	if (_node->has_attribute_or_child(TXT("offsetLocation")))
	{
		offsetLocation = Range3::zero;
		offsetLocation.load_from_xml_child_node(_node, TXT("offsetLocation"));
	}
	if (_node->has_attribute_or_child(TXT("offsetDir")))
	{
		offsetDir = Range3::zero;
		offsetDir.load_from_xml_child_node(_node, TXT("offsetDir"));
	}
	relativeVelocity.load_from_xml_child_node(_node, TXT("relativeVelocity"));
	relativePlacementVar = _node->get_name_attribute_or_from_child(TXT("relativePlacementVar"), relativePlacementVar);
	absolutePlacementVar = _node->get_name_attribute_or_from_child(TXT("absolutePlacementVar"), absolutePlacementVar);

	result &= variables.load_from_xml_child_node(_node, TXT("variables"));
	_lc.load_group_into(variables);

	{
		bool modulesAffectedByVariablesNodePresent = false;
		for_every(node, _node->children_named(TXT("modulesAffectedByVariables")))
		{
			modulesAffectedByVariablesNodePresent = true;
			for_every(mnode, node->children_named(TXT("module")))
			{
				Name var = mnode->get_name_attribute(TXT("type"));
				if (var.is_valid())
				{
					modulesAffectedByVariables.push_back(var);
				}
				else
				{
					error_loading_xml(_node, TXT("no \"type\" attribute"));
					result = false;
				}
			}
		}
		error_loading_xml_on_assert(modulesAffectedByVariablesNodePresent || variables.is_empty(), result, _node, TXT("need to provide \"modulesAffectedByVariables\" even if empty if variables used"));
	}

	error_loading_xml_on_assert(! inRoomVar.is_set() || (! socket.is_set() && !socketVar.is_set()), result, _node, TXT("can't mix inRoomVar and socket setup"));
	error_loading_xml_on_assert(!inRoomVar.is_set() || (!attached.get(false)), result, _node, TXT("can't mix inRoomVar and attached"));
	error_loading_xml_on_assert(! absolutePlacementVar.is_valid() || (! socket.is_set() && !socketVar.is_set()), result, _node, TXT("can't mix absolute placement and socket setup"));

	return result;
}

bool Elements::TemporaryObject::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= spawn.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

ScriptExecutionResult::Type Elements::TemporaryObject::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (start.is_valid() || spawn.get())
	{
		if (systemTagRequired.check(System::Core::get_system_tags()) &&
			objectVar.is_valid())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
			{
				if (auto* imo = exPtr->get())
				{
					auto* instigatorIMO = imo;
					if (instigatorObjectVar.is_valid())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(instigatorObjectVar))
						{
							if (auto* imo = exPtr->get())
							{
								instigatorIMO = imo;
							}
						}
					}

					Framework::Room* imoRoom = imo->get_presence()->get_in_room();
					Framework::Room* useRoom = imoRoom;
					{
						if (inRoomVar.is_set())
						{
							if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar.get()))
							{
								if (auto* r = exPtr->get())
								{
									useRoom = r;
								}
								else
								{
									warn(TXT("no room found for temporary spawn"));
								}
							}
							else
							{
								warn(TXT("no room var found for temporary spawn"));
							}
						}
					}
					if (auto* to = imo->get_temporary_objects())
					{
						Transform useRelativePlacement = relativePlacement;
						Optional<Transform> useAbsolutePlacement;
						if (relativePlacementVar.is_valid())
						{
							if (auto* asVec3 = _execution.get_variables().get_existing<Vector3>(relativePlacementVar))
							{
								useRelativePlacement = Transform(*asVec3, Quat::identity);
							}
							if (auto* asTransform = _execution.get_variables().get_existing<Transform>(relativePlacementVar))
							{
								useRelativePlacement = *asTransform;
							}
						}
						if (absolutePlacementVar.is_valid())
						{
							if (auto* asVec3 = _execution.get_variables().get_existing<Vector3>(absolutePlacementVar))
							{
								useAbsolutePlacement = Transform(*asVec3, Quat::identity);
							}
							if (auto* asTransform = _execution.get_variables().get_existing<Transform>(absolutePlacementVar))
							{
								useAbsolutePlacement = *asTransform;
							}
						}
						Transform setRelativePlacement = useRelativePlacement;
						Framework::IModulesOwner* spawned = nullptr;
						{
							Framework::IsPointOfInterestValid isPOIValid;
							POIManager* poiManager = nullptr;
							if (occupyPOI.is_set())
							{
								if (auto* g = Game::get_as<Game>())
								{
									auto poiUsage = occupyPOI.get();
									poiManager = &g->access_poi_manager();
									isPOIValid = [poiManager, poiUsage](Framework::PointOfInterestInstance const* _poi) { return poiManager->check(_poi, poiUsage); };
								}
							}
							Framework::PointOfInterestInstance* foundPOI = nullptr;
							if (poi.is_set())
							{
								if (auto* r = useRoom)
								{
									if (r->find_any_point_of_interest(poi.get(), foundPOI, NP, nullptr, isPOIValid))
									{
										if (attached.get(false))
										{
											useRelativePlacement = imo->get_presence()->get_placement().to_local(foundPOI->calculate_placement().to_world(setRelativePlacement));
											useAbsolutePlacement.clear();
										}
										else
										{
											useRelativePlacement = setRelativePlacement;
											useAbsolutePlacement = foundPOI->calculate_placement();
										}
									}
									else
									{
										warn(TXT("couldn't find POI to play \"%S\""), poi.get().to_char());
									}
								}
							}
							if (nearestPOI.is_set())
							{
								if (auto* r = imoRoom)
								{
									an_assert(useRoom == imoRoom);
									useRoom = imoRoom;
									Vector3 imoLoc = imo->get_presence()->get_centre_of_presence_WS();
									float foundPOIdist = 0.0f;
									Random::Generator rg;
									Range useNearestPOIAdditionalDistance = nearestPOIAdditionalDistance.get(Range(0.0f));
									r->for_every_point_of_interest(nearestPOI.get(), [useNearestPOIAdditionalDistance, &rg, &foundPOI, &foundPOIdist, imoLoc](Framework::PointOfInterestInstance* _poi)
										{
											Vector3 loc = _poi->calculate_placement().get_translation();
											float dist = (loc - imoLoc).length() + rg.get(useNearestPOIAdditionalDistance);
											if (dist < foundPOIdist ||
												!foundPOI)
											{
												foundPOI = _poi;
												foundPOIdist = dist;
											}
										}, NP, isPOIValid);
									if (foundPOI)
									{
										if (attached.get(false))
										{
											useRelativePlacement = imo->get_presence()->get_placement().to_local(foundPOI->calculate_placement().to_world(setRelativePlacement));
											useAbsolutePlacement.clear();
										}
										else
										{
											useRelativePlacement = setRelativePlacement;
											useAbsolutePlacement = foundPOI->calculate_placement();
										}
									}
									else
									{
										warn(TXT("couldn't find POI to play \"%S\""), nearestPOI.get().to_char());
									}
								}
							}
							if (occupyPOI.is_set() && foundPOI && poiManager)
							{
								if (!poiManager->check_and_occupy(foundPOI, occupyPOI.get(), occupyPOITime))
								{
									if (storeAsVar.is_valid())
									{
										_execution.access_variables().access<SafePtr<Framework::IModulesOwner>>(storeAsVar).clear();
									}
									return allowToFail? ScriptExecutionResult::Continue : ScriptExecutionResult::Repeat;
								}
							}
						}
						Optional<Name> useSocket = socket;
						if (socketVar.is_set())
						{
							if (auto* v = _execution.get_variables().get_existing<Name>(socketVar.get()))
							{
								useSocket = *v;
							}
						}
						if (! offsetLocation.is_empty())
						{
							Random::Generator rg;
							Vector3 offLoc;
							offLoc.x = rg.get(offsetLocation.x);
							offLoc.y = rg.get(offsetLocation.y);
							offLoc.z = rg.get(offsetLocation.z);
							useRelativePlacement.set_translation(useRelativePlacement.location_to_world(offLoc));
						}
						if (!offsetDir.is_empty())
						{
							Random::Generator rg;
							Rotator3 offDir;
							offDir.pitch = rg.get(offsetDir.x);
							offDir.roll = rg.get(offsetDir.y);
							offDir.yaw = rg.get(offsetDir.z);
							useRelativePlacement.set_orientation(useRelativePlacement.get_orientation().to_world(offDir.to_quat()));
						}
						Transform spawnedAtPlacement = Transform::identity;
						if (start.is_valid())
						{
							if (useAbsolutePlacement.is_set())
							{
								spawned = to->spawn_in_room(start, useRoom, useAbsolutePlacement.get().to_world(useRelativePlacement));
							}
							else
							{
								spawned = to->spawn(start, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(useSocket).at_relative_placement(useRelativePlacement));
							}
							if (spawned)
							{
								if (!relativeVelocity.is_zero())
								{
									if (auto* to = spawned->get_as_temporary_object())
									{
										to->on_activate_set_relative_velocity(relativeVelocity);
									}
								}
								spawned->access_variables().set_from(variables);
								spawned->resetup(modulesAffectedByVariables);
							}
							if (storeAbsolutePlacementAsVar.is_valid())
							{
								warn_dev_investigate(TXT("won't be able to store placement properly (not implemented)"));
							}
						}
						if (auto* toType = spawn.get())
						{
							if (auto* room = useRoom)
							{
								if (auto* subWorld = room->get_in_sub_world())
								{
									if (auto* to = subWorld->get_one_for(toType, room))
									{
										Transform placement = useAbsolutePlacement.get(imo->get_presence()->get_placement());
										if (useSocket.is_set())
										{
											if (auto* ap = imo->get_appearance())
											{
												placement = placement.to_world(ap->calculate_socket_os(useSocket.get()));
											}
										}
										placement = placement.to_world(useRelativePlacement);
										spawnedAtPlacement = placement;
										to->set_creator(imo);
										to->set_instigator(instigatorIMO);
										if (attached.get(false))
										{
											if (useSocket.is_set())
											{
												to->on_activate_attach_to_socket(imo, useSocket.get(), false, useRelativePlacement);
											}
											else
											{
												to->on_activate_attach_to(imo, false, useRelativePlacement);
											}
										}
										else
										{
											to->on_activate_place_in(room, placement);
										}
										if (!relativeVelocity.is_zero())
										{
											to->on_activate_set_relative_velocity(relativeVelocity);
										}
										to->mark_to_activate();
										to->access_variables().set_from(variables);
										to->resetup(modulesAffectedByVariables);
										spawned = to;

										{
											Framework::TemporaryObjectSpawnContext spawnContext;
											spawnContext.inRoom = room;
											spawnContext.placement = placement;

											Framework::ModuleTemporaryObjects::spawn_companions_of(imo, to, nullptr, spawnContext);
										}
									}
								}
							}
						}

						if (storeAsVar.is_valid())
						{
							_execution.access_variables().access<SafePtr<Framework::IModulesOwner>>(storeAsVar) = spawned;
						}
						if (storeAbsolutePlacementAsVar.is_valid())
						{
							_execution.access_variables().access<Transform>(storeAbsolutePlacementAsVar) = spawnedAtPlacement;
						}
					}
					else
					{
						error(TXT("no temporary objects for \"%S\""), imo->ai_get_name().to_char());
					}
				}
			}
			else
			{
				error(TXT("no object found"));
			}
		}
	}
	if (stop)
	{
		if (objectVar.is_valid())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
			{
				if (auto* imo = exPtr->get())
				{
					Framework::ParticlesUtils::desire_to_deactivate(imo);
				}
			}
		}
	}
	
	return ScriptExecutionResult::Continue;
}
