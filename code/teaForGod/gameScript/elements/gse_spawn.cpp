#include "gse_spawn.h"

#include "..\..\game\game.h"

#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\item.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// script execution variable
DEFINE_STATIC_NAME(asyncJobDone);

//

bool Spawn::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	relativeLocation.load_from_xml_child_node(_node, TXT("relLocation"));
	relativeRotation = relativeLocation.normal_2d().to_rotator() + Rotator3(0.0f, 180.0f, 0.0f);
	relativeRotation.load_from_xml_child_node(_node, TXT("relRotation"));
	{
		Rotator3 addRotation = Rotator3::zero;
		addRotation.load_from_xml_child_node(_node, TXT("addRotation"));
		relativeRotation += addRotation;
	}
	withinNav = _node->get_bool_attribute_or_from_child_presence(TXT("withinNav"), withinNav);
	
	inRoomVar = _node->get_name_attribute_or_from_child(TXT("inRoomVar"), inRoomVar);
	placementWS.load_from_xml(_node, TXT("placementWS"));
	placeAtPOI.load_from_xml(_node, TXT("placeAtPOI"));
	placeAtPOIVar.load_from_xml(_node, TXT("placeAtPOIVar"));

	inLocker = _node->get_bool_attribute_or_from_child_presence(TXT("inLocker"), inLocker);

	actorType.load_from_xml(_node, TXT("actorType"), _lc);
	itemType.load_from_xml(_node, TXT("itemType"), _lc);
	sceneryType.load_from_xml(_node, TXT("sceneryType"), _lc);

	storeVar = _node->get_name_attribute(TXT("storeVar"), storeVar);

	parameters.load_from_xml_child_node(_node, TXT("parameters"));

	return result;
}

bool Spawn::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= actorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= itemType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= sceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Spawn::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		SafePtr<Framework::GameScript::ScriptExecution> execution(&_execution);
		execution->access_variables().access<bool>(NAME(asyncJobDone)) = false;

		if (auto* game = Game::get_as<Game>())
		{
			auto relativeLocationCopy = relativeLocation;
			auto relativeRotationCopy = relativeRotation;
			auto withinNavCopy = withinNav;
			auto inLockerCopy = inLocker;
			auto itemTypeCopy = itemType.get();
			auto actorTypeCopy = actorType.get();
			auto sceneryTypeCopy = sceneryType.get();
			auto storeVarCopy = storeVar;
			auto parametersCopy = parameters;
			auto placementWSCopy = placementWS;
			Optional<Name> placeAtPOICopy = placeAtPOI;
			if (placeAtPOIVar.is_set())
			{
				if (auto* e = execution->get_variables().get_existing<Name>(placeAtPOIVar.get()))
				{
					placeAtPOICopy = *e;
				}
				else
				{
					error(TXT("no var Name:%S"), placeAtPOIVar.get().to_char());
				}
			}
			SafePtr<Framework::Room> inRoomPtr;
			if (inRoomVar.is_valid())
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar))
				{
					inRoomPtr = exPtr->get();
				}
			}
			game->add_async_world_job(TXT("spawn "),
				[execution, relativeLocationCopy, relativeRotationCopy, withinNavCopy, inLockerCopy, itemTypeCopy, actorTypeCopy, sceneryTypeCopy, storeVarCopy, parametersCopy, placementWSCopy, placeAtPOICopy, inRoomPtr]()
			{
				if (auto* game = Game::get_as<Game>())
				{
					auto* playerActor = game->access_player().get_actor();
					auto* inRoom = inRoomPtr.get();
					if (inLockerCopy)
					{
						inRoom = game->get_locker_room();
					}
					if (playerActor || inRoom)
					{
						Framework::ScopedAutoActivationGroup saag(game->get_world());

						Transform placement = Transform::identity;
						if (playerActor)
						{
							placement = playerActor->get_presence()->get_placement().to_world(Transform(relativeLocationCopy, relativeRotationCopy.to_quat()));
						}
						if (inRoom && placementWSCopy.is_set())
						{
							placement = placementWSCopy.get();
						}
						if (inRoom && placeAtPOICopy.is_set())
						{
							Framework::PointOfInterestInstance* foundPOI = nullptr;
							if (inRoom->find_any_point_of_interest(placeAtPOICopy.get(), OUT_ foundPOI))
							{
								placement = foundPOI->calculate_placement();
							}
						}

						Framework::Room* spawnInRoom = inRoom? inRoom : (playerActor? playerActor->get_presence()->get_in_room() : nullptr);

						if (withinNavCopy && spawnInRoom)
						{
							auto navNode = spawnInRoom->find_nav_location(placement);
							if (navNode.is_valid())
							{
								Vector3 loc = navNode.get_current_placement().get_translation();
								loc.z = placement.get_translation().z;
								placement.set_translation(loc);
							}
						}

						{
							Framework::ObjectType* objectType = nullptr;
							if (itemTypeCopy)
							{
								objectType = itemTypeCopy;
							}
							if (actorTypeCopy)
							{
								objectType = actorTypeCopy;
							}
							if (sceneryTypeCopy)
							{
								objectType = sceneryTypeCopy;
							}
							if (objectType)
							{	// copied from point of interest instance "spawn" functionality
								RefCountObjectPtr<Framework::DelayedObjectCreation> doc(new Framework::DelayedObjectCreation());
								doc->priority = 10000; // the most important thing to perform

								if (!doc->inSubWorld && spawnInRoom)
								{
									doc->inSubWorld = spawnInRoom->get_in_sub_world();
								}
								if (!doc->inSubWorld && playerActor)
								{
									doc->inSubWorld = playerActor->get_in_sub_world();
								}
								doc->inRoom = spawnInRoom;
								doc->name = TXT("game script object");
								doc->objectType = objectType;
								doc->placement = placement;
								doc->randomGenerator = Random::Generator();

								if (spawnInRoom) // for items created inside we don't have a room to use
								{
									spawnInRoom->collect_variables(doc->variables);
								}

								doc->variables.set_from(parametersCopy);

								// do not delay, do it immediately
								doc->process(Game::get(), true);

								if (auto* obj = doc->createdObject.get())
								{
									if (storeVarCopy.is_valid())
									{
										execution->access_variables().access< SafePtr<Framework::IModulesOwner> >(storeVarCopy) = doc->createdObject.get();
									}

									if (inLockerCopy)
									{
										obj->suspend_advancement();
									}
								}
							}
							else
							{
								error(TXT("no object to spawn!"));
							}
						}
					}
				}
				if (auto* ex = execution.get())
				{
					ex->access_variables().access<bool>(NAME(asyncJobDone)) = true;
				}
			});
		}
	}

	if (_execution.get_variables().get_value<bool>(NAME(asyncJobDone), false))
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}
