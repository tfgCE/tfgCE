#include "gse_waitForLook.h"

#include "..\..\game\game.h"
#include "..\..\loaders\hub\loaderHubScreen.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_customDraw.h"
#include "..\..\overlayInfo\elements\oie_text.h"
#include "..\..\sound\subtitleSystem.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\system\core.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\framework\gameScript\gameScript.h"
#include "..\..\..\framework\gameScript\elements\gse_label.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\text\localisedString.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// game input definition
DEFINE_STATIC_NAME(tutorial);

// game input
DEFINE_STATIC_NAME(continue);

//

// localised strings
DEFINE_STATIC_NAME_STR(lsTutorialContinue, TXT("tutorials; continue"));
DEFINE_STATIC_NAME_STR(lsTutorialContinueToEnd, TXT("tutorials; continue to end"));

// variables / ids
DEFINE_STATIC_NAME(timeWaitingForLook);
DEFINE_STATIC_NAME(waitForLook);
DEFINE_STATIC_NAME(lookInDir);
DEFINE_STATIC_NAME(lookingAt);
DEFINE_STATIC_NAME(remainInvisible);
DEFINE_STATIC_NAME(lookAtPOI);

// object tags
DEFINE_STATIC_NAME(palace);

//

bool WaitForLook::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	justClear.load_from_xml(_node, TXT("justClear"));

	if (justClear.get(false))
	{
		// leave as it is
	}
	else
	{
		time.load_from_xml(_node, TXT("time"));
		timeLimit.load_from_xml(_node, TXT("timeLimit"));
		goToLabelOnTimeLimit = _node->get_name_attribute_or_from_child(TXT("goToLabelOnTimeLimit"), goToLabelOnTimeLimit);
		prompt.load_from_xml(_node, TXT("prompt"));

		distance.load_from_xml(_node, TXT("distance"));

		{
			String const& atString = _node->get_string_attribute(TXT("at"));
			if (atString == TXT("hands"))
			{
				target = Target::Hands;
			}
			else if (atString == TXT("pockets"))
			{
				target = Target::Pockets;
			}
			else if (atString == TXT("exm trays"))
			{
				target = Target::EXMTrays;
			}
			else if (atString == TXT("fads"))
			{
				target = Target::FADs;
			}
			else if (atString == TXT("main equipment"))
			{
				target = Target::MainEquipment;
			}
			else if (atString == TXT("palace"))
			{
				target = Target::Palace;
			}
			else
			{
				target = Target::None;
				if (!atString.is_empty())
				{
					error_loading_xml(_node, TXT("invalid \"at\""));
				}
			}
		}

		atHubScreen.load_from_xml(_node, TXT("atHubScreen"));
		if (atHubScreen.is_set())
		{
			target = Target::HubScreen;
		}

		atPOI.load_from_xml(_node, TXT("atPOI"));
		if (atPOI.is_set())
		{
			target = Target::POI;
		}

		atClosestPOI.load_from_xml(_node, TXT("atClosestPOI"));
		if (atClosestPOI.is_set())
		{
			target = Target::POI;
		}

		atVar.load_from_xml(_node, TXT("atVar"));
		if (atVar.is_set())
		{
			target = Target::ObjectFromVar;
		}
		socket.load_from_xml(_node, TXT("socket"));

		radius.load_from_xml(_node, TXT("radius"));
		angle.load_from_xml(_node, TXT("angle"));
		offsetWS.load_from_xml(_node, TXT("offsetWS"));

		endless = _node->get_bool_attribute_or_from_child_presence(TXT("endless"), endless);
	}

	return result;
}

float calculate_look_in_dir(Vector3 _relTarget)
{
	float yThreshold = 0.2f;
	if (_relTarget.y < yThreshold)
	{
		_relTarget.z *= max(0.0f, 1.0f - (yThreshold - _relTarget.y) * 1.4f);
	}
	_relTarget.y = 0.0f;
	_relTarget.normalise();
	swap(_relTarget.y, _relTarget.z);
	return _relTarget.to_rotator().yaw;
}

Framework::GameScript::ScriptExecutionResult::Type WaitForLook::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (justClear.get(false))
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->deactivate(NAME(waitForLook), true);
		}
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}

	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		_execution.access_variables().access<float>(NAME(timeWaitingForLook)) = 0.0f;
		_execution.access_variables().access<float>(NAME(waitForLook)) = 0.0f;
		_execution.access_variables().access<bool>(NAME(remainInvisible)) = false;
		_execution.access_variables().access<SafePtr<Framework::PointOfInterestInstance>>(NAME(lookAtPOI)).clear();
		if (prompt.get(true))//TutorialSystem::check_active()))
		{
			if (auto* ois = OverlayInfo::System::get())
			{
				{
					auto* c = new OverlayInfo::Elements::CustomDraw();

					c->with_id(NAME(waitForLook));
					c->set_cant_be_deactivated_by_system();

					//c->with_pulse();
					c->with_draw(
						[&_execution](float _active, float _pulse, Colour const& _colour)
						{
							if (!_execution.get_variables().get_value<bool>(NAME(remainInvisible), false))
							{
								Colour colour = _colour.mul_rgb(_pulse).with_alpha(_active);

								float lookAt = _execution.get_variables().get_value<float>(NAME(waitForLook), 0.0f);
							
								float radius = 1.2f;
								float innerSafeRadius = radius * 0.6f;
								::System::Video3DPrimitives::ring_xz(colour, Vector3::zero, radius * 0.8f, radius * 1.0f, _active < 1.0f, 0.0f, lookAt, radius * 0.05f);
								//::System::Video3DPrimitives::fill_circle_xz(_colour.mul_rgb(_pulse).with_alpha(_active), Vector3::zero, 1.0f * lookAt, _active < 1.0f);

								if (! _execution.get_variables().get_value<bool>(NAME(lookingAt), true))
								{
									float lookInDir = _execution.get_variables().get_value<float>(NAME(lookInDir), 0.0f);
									ArrayStatic<Vector3, 20> points; SET_EXTRA_DEBUG_INFO(points, TXT("WaitForLook::execute.points"));
									points.push_back(Vector3( 0.0f, 0.0f,  1.0f));
									points.push_back(Vector3( 1.0f, 0.0f,  0.0f));
									points.push_back(Vector3( 0.5f, 0.0f,  0.0f));
									points.push_back(Vector3( 0.5f, 0.0f, -1.0f));
									points.push_back(Vector3(-0.5f, 0.0f, -1.0f));
									points.push_back(Vector3(-0.5f, 0.0f,  0.0f));
									points.push_back(Vector3(-1.0f, 0.0f,  0.0f));
									points.push_back(points.get_first()); // loop it

									for_every(p, points)
									{
										*p = p->rotated_by_roll(lookInDir);
										*p = *p * innerSafeRadius;
									}

									::System::Video3DPrimitives::line_strip(colour, points.get_data(), points.get_size());
								}
							}
						});

					bool showCloser = SubtitleSystem::withSubtitles;
					if (!showCloser)
					{
						if (auto* game = Game::get_as<Game>())
						{
							if (auto* playerActor = game->access_player().get_actor())
							{
								if (auto* pilgrim = playerActor->get_gameplay_as<ModulePilgrim>())
								{
									if (pilgrim->access_overlay_info().is_showing_main())
									{
										showCloser = true;
									}
								}
							}
						}
					}

					if (true)
					{
						Rotator3 offset = Rotator3(0.0f, 0.0f, 0.0f);
						c->with_location(OverlayInfo::Element::Relativity::RelativeToCamera);
						c->with_rotation(OverlayInfo::Element::Relativity::RelativeToCamera, offset);
						c->with_distance(distance.get(showCloser? 0.6f : 10.0f));
					}
					else
					{
						Rotator3 offset = Rotator3(-10.0f, 0.0f, 0.0f);
						if (TutorialSystem::check_active())
						{
							TutorialSystem::configure_oie_element(c, offset);
						}
						else
						{
							c->with_location(OverlayInfo::Element::Relativity::RelativeToAnchor);
							c->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchor, offset);
							c->with_distance(distance.get(10.0f));
						}
					}

					ois->add(c);
				}
			}
		}
	}

	float lookInDir = 0.0f;
	float lookAt = _execution.get_variables().get_value<float>(NAME(waitForLook), 0.0f);
	float deltaTime = TutorialSystem::check_active() ? ::System::Core::get_raw_delta_time() : ::System::Core::get_delta_time();

	bool lookingAt = true;
	bool remainInvisible = false;
	if (target == Target::HubScreen)
	{
		lookingAt = true;
		remainInvisible = false;
		if (TutorialSystem::check_active() && atHubScreen.is_set())
		{
			if (auto* hub = TutorialSystem::get()->get_active_hub())
			{
				if (auto* screen = hub->get_screen(atHubScreen.get()))
				{
					lookingAt = false;
					Transform view = hub->get_last_view_placement();
					Vector3 diff = screen->calculate_screen_centre().get_translation() - view.get_translation();
					Vector3 viewDir = view.vector_to_world(Vector3(0.0f, 1.0f, 0.0f)).normal();
					lookInDir = calculate_look_in_dir(view.vector_to_local(diff));
					float diffYaw = Rotator3::normalise_axis(Rotator3::get_yaw(viewDir) - Rotator3::get_yaw(diff));
					float diffPitch = Rotator3::normalise_axis(viewDir.to_rotator().pitch - diff.to_rotator().pitch);
					float dot = Vector3::dot(viewDir.normal(), diff.normal());
					float radius = min(screen->size.x * 0.5f, screen->size.y * 0.5f);
					if (dot > cos_deg(radius) ||
						(abs(diffYaw) < screen->size.x * 0.5f &&
						 abs(diffPitch) < screen->size.y * 0.5))
					{
						lookingAt = true;
					}
				}
			}
		}
	}
	else
	{
		if (TutorialSystem::get_active_hub())
		{
			// to exit immediately
			lookingAt = true;
			lookAt = 1.0f;
		}
		else
		{
			remainInvisible = false;
			if (auto* game = Game::get_as<Game>())
			{
				if (auto* playerActor = game->access_player().get_actor())
				{
					if (auto* presence = playerActor->get_presence())
					{
						Framework::Room* inRoom = nullptr;
						inRoom = presence->get_in_room();
						Transform eyesWS = presence->get_placement().to_world(presence->get_eyes_relative_look());
						Vector3 viewAtWS = eyesWS.get_translation();
						Vector3 viewDirWS = eyesWS.vector_to_world(Vector3(0.0f, 1.0f, 0.0f)).normal();
						Transform viewTransformWS = eyesWS;
						Optional<Vector3> atWS;
						Optional<float> useRadius;
						if (target == Target::POI && (atPOI.is_set() || atClosestPOI.is_set()))
						{
							auto & lookAtPOI = _execution.access_variables().access<SafePtr<Framework::PointOfInterestInstance>>(NAME(lookAtPOI));
							if (!lookAtPOI.is_pointing_at_something())
							{
								if (!lookAtPOI.is_pointing_at_something() && atClosestPOI.is_set())
								{
									Framework::PointOfInterestInstance* foundPOI = nullptr;
									float bestDist = 0.0f;
									inRoom->for_every_point_of_interest(atClosestPOI, [&bestDist, &foundPOI, viewAtWS](Framework::PointOfInterestInstance* _poi)
										{
											Vector3 at = _poi->calculate_placement().get_translation();
											float dist = (at - viewAtWS).length_squared();
											if (dist < bestDist || !foundPOI)
											{
												foundPOI = _poi;
												bestDist = dist;
											}
										}, true);
									if (foundPOI)
									{
										lookAtPOI = foundPOI;
									}
								}
								if (!lookAtPOI.is_pointing_at_something() && atPOI.is_set())
								{
									Framework::PointOfInterestInstance* foundPOI = nullptr;
									if (inRoom->find_any_point_of_interest(atPOI, foundPOI, true))
									{
										lookAtPOI = foundPOI;
									}
								}
							}
							if (auto* poi = lookAtPOI.get())
							{
								lookingAt = false;
								atWS = poi->calculate_placement().get_translation();
								useRadius = radius.get(0.5f);
							}
						}
						if (target == Target::ObjectFromVar && atVar.is_set())
						{
							if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(atVar.get()))
							{
								if (auto* imo = exPtr->get())
								{
									lookingAt = false;
									atWS = imo->get_presence()->get_centre_of_presence_WS();
									if (socket.is_set())
									{
										atWS = imo->get_presence()->get_placement().location_to_world(imo->get_appearance()->calculate_socket_os(socket.get()).get_translation());
									}
									useRadius = radius.get(imo->get_presence()->get_presence_radius());
								}
							}
							if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Door>>(atVar.get()))
							{
								if (auto* door = exPtr->get())
								{
									//FOR_EVERY_DOOR_IN_ROOM(dir, inRoom)
									for_every_ptr(dir, inRoom->get_doors())
									{
										if (dir->get_door() == door)
										{
											lookingAt = false;
											atWS = dir->get_hole_centre_WS();
											useRadius = radius.get(0.5f);
										}
									}
								}
							}
						}
						if (target == Target::Pockets)
						{
							lookingAt = false;
							atWS = presence->get_placement().location_to_world(Vector3(0.0f, 0.0f, 0.0f)); // should be enough to make you look down
						}
						if (target == Target::Palace)
						{
							lookingAt = false;
							if (auto* r = presence->get_in_room())
							{
								for_every_ptr(obj, r->get_objects())
								{
									if (obj->get_tags().get_tag(NAME(palace)))
									{
										atWS = obj->get_presence()->get_placement().get_translation();
									}
								}
							}
						}
						// be the same for time being
						if (target == Target::EXMTrays ||
							target == Target::FADs)
						{
							lookingAt = false;
							if (auto* pilgrim = playerActor->get_gameplay_as<ModulePilgrim>())
							{
								Vector3 atWSHand[2];
								float dotHand[2];
								for_count(int, hIdx, Hand::MAX)
								{
									Hand::Type hand = (Hand::Type)hIdx;
									atWSHand[hand] = presence->get_placement().location_to_world(pilgrim->get_forearm_location_os(hand));
									dotHand[hand] = Vector3::dot(viewDirWS, (atWSHand[hand] - viewAtWS).normal());
								}
								if (dotHand[Hand::Left] > dotHand[Hand::Right])
								{
									atWS = atWSHand[Hand::Left];
								}
								else
								{
									atWS = atWSHand[Hand::Right];
								}
							}
						}
						if (target == Target::MainEquipment)
						{
							lookingAt = false;
							if (auto* pilgrim = playerActor->get_gameplay_as<ModulePilgrim>())
							{
								Optional<Vector3> atWSHand[2];
								float dotHand[2];
								dotHand[0] = -2.0f;
								dotHand[1] = -2.0f;
								for_count(int, hIdx, Hand::MAX)
								{
									Hand::Type hand = (Hand::Type)hIdx;
									if (auto* meq = pilgrim->get_main_equipment(hand))
									{
										if (auto* meqPresence = meq->get_presence())
										{
											if (meqPresence->get_in_room() == presence->get_in_room())
											{
												// this works only if in the same room
												atWSHand[hand] = meqPresence->get_centre_of_presence_WS();
											}
											else if (auto* path = meq->get_presence()->get_path_to_attached_to())
											{
												atWSHand[hand] = path->location_from_owner_to_target(meq->get_presence()->get_centre_of_presence_WS());
											}
											if (atWSHand[hand].is_set())
											{
												dotHand[hand] = Vector3::dot(viewDirWS, (atWSHand[hand].get() - viewAtWS).normal());
											}
										}
									}
								}
								if (dotHand[Hand::Left] > dotHand[Hand::Right])
								{
									if (atWSHand[Hand::Left].is_set())
									{
										atWS = atWSHand[Hand::Left];
									}
								}
								else
								{
									if (atWSHand[Hand::Right].is_set())
									{
										atWS = atWSHand[Hand::Right];
									}
								}
							}
						}
						if (atWS.is_set())
						{
							atWS = atWS.get() + offsetWS.get(Vector3::zero);
							Vector3 diff = atWS.get() - viewAtWS;
							lookInDir = calculate_look_in_dir(viewTransformWS.vector_to_local(diff));

							float treAngle = angle.get(atan_deg(useRadius.get(radius.get(0.1f)) / diff.length()));
							float dotDir = Vector3::dot(viewDirWS, (atWS.get() - viewAtWS).normal());
							float threshold = cos_deg(max(5.0f, treAngle));
							if (dotDir > threshold)
							{
								lookingAt = true;
							}
						}
						else
						{
							remainInvisible = true;
						}
					}
				}
			}
		}
		if (auto* vr = VR::IVR::get())
		{
			todo_note(TXT("look at for hands only for vr"));
			if (vr->is_controls_view_valid())
			{
				Vector3 viewAt = vr->get_controls_view().get_translation();
				Vector3 viewDir = vr->get_controls_view().vector_to_world(Vector3(0.0f, 1.0f, -0.05f)).normal();
				Transform viewTransform = look_matrix(viewAt, viewDir, vr->get_controls_view().vector_to_local(Vector3::zAxis).normal()).to_transform();
				if (target == Target::Hands)
				{
					lookingAt = false;
					float closer = -2.0f;
					float closerLookInDir = 0.0f;
					for_count(int, handIdx, Hand::MAX)
					{
						Hand::Type hand = (Hand::Type)handIdx;
						if (vr->is_hand_available(hand))
						{
							auto& handPlacement = vr->get_controls_pose_set().get_hand(hand).placement;
							if (handPlacement.is_set())
							{
								Vector3 handAt = handPlacement.get().to_world(VR::VRHandPose::get_hand_centre_offset(hand)).get_translation();
								float d = Vector3::dot(viewDir, (handAt - viewAt).normal());
								if (d >= closer)
								{
									closer = d;
									closerLookInDir = calculate_look_in_dir(viewTransform.vector_to_local(handAt - viewAt));
								}
								if (d > cos_deg(20.0f))
								{
									lookingAt = true;
									break;
								}
							}
						}
					}
					lookInDir = closerLookInDir;
				}
			}
		}
		else
		{
			if (target == Target::Hands ||
				target == Target::FADs ||
				target == Target::EXMTrays ||
				target == Target::MainEquipment)
			{
				// if not in vr, just skip it
				//lookingAt = true;
			}
		}
	}

	float const speed = 1.0f / time.get(TutorialSystem::check_active()? 0.5f : 0.2f);
	lookAt = clamp(lookAt + (lookingAt ? 1.0f : -1.0f) * speed * deltaTime, 0.0f, 1.0f);

	_execution.access_variables().access<float>(NAME(waitForLook)) = lookAt;
	_execution.access_variables().access<float>(NAME(lookInDir)) = lookInDir;
	_execution.access_variables().access<bool>(NAME(lookingAt)) = lookingAt;
	_execution.access_variables().access<bool>(NAME(remainInvisible)) = remainInvisible;

	if (_execution.access_variables().access<float>(NAME(timeWaitingForLook)) < 0.01f && lookingAt)
	{
		// end immediately if we started already looking at it
		lookAt = 1.0f;
	}
	else
	{
		_execution.access_variables().access<float>(NAME(timeWaitingForLook)) += deltaTime;

		if (timeLimit.is_set())
		{
			if (_execution.get_variables().get_value<float>(NAME(timeWaitingForLook), 0.0f) >= timeLimit.get())
			{
				if (auto* ois = OverlayInfo::System::get())
				{
					ois->deactivate(NAME(waitForLook), true);
				}
				if (goToLabelOnTimeLimit.is_valid())
				{
					int labelAt = NONE;
					if (auto* script = _execution.get_script())
					{
						for_every_ref(e, script->get_elements())
						{
							if (auto* label = fast_cast<Framework::GameScript::Elements::Label>(e))
							{
								if (label->get_id() == goToLabelOnTimeLimit)
								{
									labelAt = for_everys_index(e);
									break;
								}
							}
						}
					}

					if (labelAt != NONE)
					{
						_execution.set_at(labelAt);
					}
					else
					{
						error(TXT("did not find label \"%S\""), goToLabelOnTimeLimit.to_char());
					}

					if (auto* ois = OverlayInfo::System::get())
					{
						ois->deactivate(NAME(waitForLook), true);
					}

					return Framework::GameScript::ScriptExecutionResult::SetNextInstruction;
				}
				else
				{
					return Framework::GameScript::ScriptExecutionResult::Continue;
				}
			}
		}
	}

	if (lookAt >= 1.0f && !endless)
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->deactivate(NAME(waitForLook), true);
		}
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}

void WaitForLook::interrupted(Framework::GameScript::ScriptExecution& _execution) const
{
	base::interrupted(_execution);
	if (auto* ois = OverlayInfo::System::get())
	{
		ois->deactivate(NAME(waitForLook), true);
	}
}

