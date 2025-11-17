#include "gsc_pilgrim.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

// "holding" values
DEFINE_STATIC_NAME(upgradeCanister);

// room tags
DEFINE_STATIC_NAME(openWorldExterior);

// POIs
DEFINE_STATIC_NAME_STR(environmentAnchor, TXT("environment anchor"));

//

bool Conditions::Pilgrim::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	holding.load_from_xml(_node, TXT("holding"));
	relativeToEnvironment = _node->get_bool_attribute_or_from_child_presence(TXT("relativeToEnvironment"), relativeToEnvironment);
	isLookingAtObjectVar.load_from_xml(_node, TXT("isLookingAtObjectVar"));
	lookingAtAngle.load_from_xml(_node, TXT("lookingAtAngle"));
	lookingAtRadius.load_from_xml(_node, TXT("lookingAtRadius"));
	useVRSpaceBasedDoorsCheck.load_from_xml(_node, TXT("useVRSpaceBasedDoorsCheck"));
	looksUpAngle.load_from_xml(_node, TXT("looksUpAngle"));
	looksYawAt.load_from_xml(_node, TXT("looksYawAt"));
	looksYawMaxOff.load_from_xml(_node, TXT("looksYawMaxOff"));
	looksPitchAt.load_from_xml(_node, TXT("looksPitchAt"));
	looksPitchMaxOff.load_from_xml(_node, TXT("looksPitchMaxOff"));
	inNavConnectorJunctionRoom.load_from_xml(_node, TXT("inNavConnectorJunctionRoom"));
	atCell.load_from_xml(_node, TXT("atCell"));
	atCellX.load_from_xml(_node, TXT("atCellX"));
	atCellY.load_from_xml(_node, TXT("atCellY"));
	inRoomVar = _node->get_name_attribute_or_from_child(TXT("inRoomVar"), inRoomVar);
	inRoomTagged.load_from_xml_attribute_or_child_node(_node, TXT("inRoomTagged"));

	return result;
}

bool Conditions::Pilgrim::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (auto* g = Game::get_as<Game>())
	{
		todo_multiplayer_issue(TXT("we assume sp"));
		if (auto* pa = g->access_player().get_actor())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				if (holding.is_set())
				{
					bool holds = false;
					for_count(int, hIdx, Hand::MAX)
					{
						Hand::Type hand = (Hand::Type)hIdx;
						Framework::IModulesOwner* imo = nullptr;
						if (!imo)
						{
							imo = mp->get_grabbed_object(hand);
						}
						if (!imo)
						{
							imo = mp->get_hand_equipment(hand);
						}
						Framework::Object* imoObject = imo->get_as_object();
						if (imo)
						{
							if (holding == NAME(upgradeCanister))
							{
								if (imoObject && imoObject->get_tags().get_tag(NAME(upgradeCanister)))
								{
									holds = true;
									break;
								}
							}
							else
							{
								todo_implement;
							}
						}
					}
					anyOk |= holds;
					anyFailed |= !holds;
				}
				if (looksUpAngle.is_set())
				{
					if (auto* p = pa->get_presence())
					{
						Transform headBoneOS = Transform::identity;
						if (p->is_vr_movement())
						{
							headBoneOS = p->get_requested_relative_look_for_vr();
						}
						else
						{
							headBoneOS = p->get_requested_relative_look();
						}
						float pitch = headBoneOS.get_axis(Axis::Forward).to_rotator().pitch;

						bool ok = pitch >= looksUpAngle.get();

						anyOk |= ok;
						anyFailed |= !ok;
					}
				}
				if (auto* p = pa->get_presence())
				{
					Transform headBoneOS = Transform::identity;
					if (p->is_vr_movement())
					{
						headBoneOS = p->get_requested_relative_look_for_vr();
					}
					else
					{
						headBoneOS = p->get_requested_relative_look();
					}
					Transform headBoneWS = p->get_placement().to_world(headBoneOS);
					if (relativeToEnvironment)
					{
						if (auto* r = p->get_in_room())
						{
							if (r->get_tags().has_tag(NAME(openWorldExterior)))
							{
								bool found = false;
								{
									Framework::PointOfInterestInstance* foundPOI = nullptr;
									if (!found && r->find_any_point_of_interest(NAME(environmentAnchor), foundPOI))
									{
										Transform ea = foundPOI->calculate_placement();
										headBoneWS = ea.to_local(headBoneWS);
										found = true;
									}
								}
								if (!found)
								{
									if (auto* rt = r->get_room_type())
									{
										if (!found && rt->get_use_environment().anchorPOI.is_valid())
										{
											Framework::PointOfInterestInstance* foundPOI = nullptr;
											if (r->find_any_point_of_interest(rt->get_use_environment().anchorPOI, foundPOI))
											{
												Transform ea = foundPOI->calculate_placement();
												headBoneWS = ea.to_local(headBoneWS);
												found = true;
											}
										}
										if (!found && rt->get_use_environment().anchor.is_set())
										{
											Transform placement = rt->get_use_environment().anchor.get(r, Transform::zero, AllowToFail);
											if (! placement.is_zero())
											{
												headBoneWS = placement.to_local(headBoneWS);
												found = true;
											}
										}
									}
								}
							}
						}
					}
					Rotator3 headBoneWSFwd = headBoneWS.get_axis(Axis::Forward).to_rotator();
					for_count(int, angleIdx, 2)
					{
						if ((angleIdx == 0 && looksYawAt.is_set()) ||
							(angleIdx == 1 && looksPitchAt.is_set()))
						{
							float angleAt = 0.0f;
							float angleMaxOff = 0.0f;
							if (angleIdx == 0)
							{
								angleAt = looksYawAt.get();
								angleMaxOff = looksYawMaxOff.get(45.0f);
							}
							else
							{
								angleAt = looksPitchAt.get();
								angleMaxOff = looksPitchMaxOff.get(45.0f);
							}
							float angle = angleAt;
							if (angleIdx == 0)
							{
								angle = headBoneWSFwd.yaw;
							}
							else
							{
								angle = headBoneWSFwd.pitch;
							}

							float diff = angle - angleAt;
							diff = Rotator3::normalise_axis(diff);
						
							bool ok = abs(diff) < angleMaxOff;

							anyOk |= ok;
							anyFailed |= !ok;
						}
					}
					if (isLookingAtObjectVar.is_set())
					{
						bool ok = false;
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(isLookingAtObjectVar.get()))
						{
							if (auto* atIMO = exPtr->get())
							{
								if (useVRSpaceBasedDoorsCheck.get(true))
								{
									Transform atWS = atIMO->get_presence()->get_centre_of_presence_transform_WS();
									Framework::Room* atRoom = atIMO->get_presence()->get_in_room();
									Transform atVRS = atRoom->get_vr_anchor(atWS).to_local(atWS);

									Transform fromWS = headBoneWS;
									Framework::Room* fromRoom = p->get_in_room();
									Transform fromAnchorVRWS = fromRoom->get_vr_anchor(fromWS);
									Transform atFromWS = fromAnchorVRWS.to_world(atVRS);

									Transform endWS = atFromWS;

									Vector3 atLS = fromWS.location_to_local(endWS.get_translation());
									float dotAngle = Vector3::dot(Vector3::yAxis, atLS.normal());
									float angleFromRadius = atan_deg(lookingAtRadius.get(0.0f) / atLS.length());
									if (dotAngle >= cos_deg(lookingAtAngle.get(20.0f) + angleFromRadius))
									{
										Framework::Room* endRoom = nullptr;
										if (fromRoom->move_through_doors(fromWS, REF_ endWS, endRoom))
										{
											if (atRoom == endRoom &&
												Vector3::distance(atFromWS.get_translation(), endWS.get_translation()) < 0.1f)
											{
												ok = true;
											}
										}
									}
								}
								else
								{
									warn_dev_investigate(TXT("not implemented (gsc:pilgrim:isLookingAtObjectVar without useVRSpaceBasedDoorsCheck"));
								}
							}
						}
						anyOk |= ok;
						anyFailed |= !ok;
					}
				}
				if (inNavConnectorJunctionRoom.is_set())
				{
					if (auto* p = pa->get_presence())
					{
						bool ok = false;
						if (auto* r = p->get_in_room())
						{
							int navConnectors = 0;
							for_every_ptr(door, r->get_doors())
							{
								if (!door->is_visible()) continue;
								if (auto* d = door->get_door())
								{
									if (auto* dt = d->get_door_type())
									{
										if (dt->is_nav_connector())
										{
											++navConnectors;
										}
									}
								}
							}
							if (navConnectors >= 3)
							{
								ok = true;
							}
						}

						anyOk |= ok;
						anyFailed |= !ok;
					}
				}
				if (atCell.is_set() ||
					atCellX.is_set() ||
					atCellY.is_set())
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						auto pat = piow->find_cell_at(pa);
						bool ok = false;
						if (pat.is_set())
						{
							bool anyCellFailed = false;
							if (atCell.is_set())
							{
								bool cellOk = pat.get() == atCell.get();
								anyCellFailed |= ! cellOk;
							}
							if (atCellX.is_set())
							{
								bool cellOk = atCellX.get().does_contain(pat.get().x);
								anyCellFailed |= ! cellOk;
							}
							if (atCellY.is_set())
							{
								bool cellOk = atCellY.get().does_contain(pat.get().y);
								anyCellFailed |= !cellOk;
							}
							ok = !anyCellFailed;
						}
						anyOk |= ok;
						anyFailed |= !ok;
					}
				}
				if (inRoomVar.is_valid())
				{
					bool ok = false;
					if (auto* p = pa->get_presence())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar))
						{
							if (auto* r = exPtr->get())
							{
								ok = r == p->get_in_room();
							}
						}
						anyOk |= ok;
						anyFailed |= !ok;
					}
				}
				if (! inRoomTagged.is_empty())
				{
					if (auto* p = pa->get_presence())
					{
						bool ok = false;
						if (auto* r = p->get_in_room())
						{
							if (inRoomTagged.check(r->get_tags()))
							{
								ok = true;
							}
						}

						anyOk |= ok;
						anyFailed |= !ok;
					}
				}
			}
		}
	}

	return anyOk && !anyFailed;
}
