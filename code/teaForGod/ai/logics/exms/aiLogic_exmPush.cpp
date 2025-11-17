#include "aiLogic_exmPush.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\custom\mc_pickup.h"
#include "..\..\..\modules\custom\mc_switchSidesHandler.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\utils\collectInTargetCone.h"

#include "..\..\..\..\core\containers\arrayStack.h"

#include "..\..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\sound\soundScene.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\world.h"

#include "..\..\..\..\core\debug\debugRenderer.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_DRAW_PUSHING

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// message
DEFINE_STATIC_NAME_STR(aimConfussion, TXT("confussion"));

// sounds
DEFINE_STATIC_NAME(push);

// variables
DEFINE_STATIC_NAME(confussionDuration);
DEFINE_STATIC_NAME(coneAngle);
DEFINE_STATIC_NAME(maxOffDistance);
DEFINE_STATIC_NAME(distance);
DEFINE_STATIC_NAME(changeMovement);
DEFINE_STATIC_NAME(velocity);
DEFINE_STATIC_NAME(withTag);
DEFINE_STATIC_NAME(allowAttachedWithTag);

// tags
DEFINE_STATIC_NAME(pushTarget);

// movement names
DEFINE_STATIC_NAME(attached);
DEFINE_STATIC_NAME(thrown);
DEFINE_STATIC_NAME(flying);

//

REGISTER_FOR_FAST_CAST(EXMPush);

EXMPush::EXMPush(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMPush::~EXMPush()
{
}

void EXMPush::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	confussionDuration = _parameters.get_value(NAME(confussionDuration), 0.0f);
	coneAngle = _parameters.get_value(NAME(coneAngle), 80.0f);
	maxOffDistance = _parameters.get_value(NAME(maxOffDistance), 0.5f);
	distance = _parameters.get_value(NAME(distance), 5.0f);
	changeMovement = _parameters.get_value(NAME(changeMovement), false);
	velocity = _parameters.get_value(NAME(velocity), Vector3::yAxis * 10.0f);
	withTag = _parameters.get_value(NAME(withTag), NAME(pushTarget));
	allowAttachedWithTag = _parameters.get_value(NAME(allowAttachedWithTag), Name::invalid());
}

void EXMPush::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX)
	{
		if (auto* gd = GameDirector::get())
		{
			//if (!gd->game_is_no_violence_forced())
			{
				if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
					pilgrimModule->is_active_exm_equipped(hand, owner.get()))
				{
					bool control = exmModule->mark_exm_active_blink();

					if (control)
					{
						Transform startingPlacementWS;

						ARRAY_STACK(CollectInTargetCone::Entry, collected, 32);
						{
							CollectInTargetCone::Params citcParams;
							citcParams.not_attached();
							citcParams.attached_with_tag(allowAttachedWithTag);
							citcParams.with_tag(withTag);

							auto* p = pilgrimOwner->get_presence();
							Transform placement = p->get_placement();
							placement = placement.to_world(p->get_eyes_relative_look());
							
							startingPlacementWS = placement;

							Vector3 startLoc = placement.get_translation();
							Vector3 dir = placement.get_axis(Axis::Forward);

							Framework::Room* startInRoom = p->get_in_room();
							Transform intoRoomTransform = Transform::identity;

							startInRoom->move_through_doors(Transform(p->get_centre_of_presence_WS(), Quat::identity), Transform(startLoc, Quat::identity), OUT_ startInRoom, OUT_ intoRoomTransform);

							CollectInTargetCone::collect(intoRoomTransform.location_to_local(startLoc), intoRoomTransform.vector_to_local(dir),
								distance, coneAngle * 0.5f, maxOffDistance, startInRoom, distance, collected, nullptr, intoRoomTransform, citcParams);
						}

						for_every(c, collected)
						{
							if (auto* imo = c->target)
							{
								bool isAttached = imo->get_presence()->is_attached() || imo->get_movement_name() == NAME(attached);
								if (isAttached)
								{
									// check if it is in the pocket or holder, leave it then
									if (auto* pickup = imo->get_custom<CustomModules::Pickup>())
									{
										if (pickup->is_held() || pickup->is_in_pocket() || pickup->is_in_holder())
										{
											continue;
										}
									}
								}

								Vector3 dir = (c->closestLocationWS - startingPlacementWS.get_translation()).normal();
								Transform pushTransform = look_matrix(Vector3::zero, dir, startingPlacementWS.get_axis(Axis::Up)).to_transform();

								float powerPt = clamp((c->closestLocationWS - startingPlacementWS.get_translation()).length() / distance, 0.0f, 1.0f);
								powerPt = sqr(powerPt);
								powerPt = 1.0f - powerPt;

								if (auto* obj = imo->get_as_object())
								{
									powerPt *= obj->get_tags().get_tag(withTag);
								}

								if (isAttached)
								{
									imo->get_presence()->detach();
								}

								if (isAttached || changeMovement)
								{
									if (!imo->activate_movement(NAME(thrown), true /*may fail*/))
									{
										imo->activate_movement(NAME(flying), true /*may fail*/);
									}
								}

								imo->get_presence()->add_velocity_impulse((c->intoRoomTransform.to_local(pushTransform)).vector_to_world(velocity * powerPt));

#ifdef DEBUG_DRAW_PUSHING
								debug_context(pilgrimOwner->get_presence()->get_in_room());
								debug_draw_time_based(3.0f, debug_draw_arrow(true, Colour::red, startingPlacementWS.get_translation() + Vector3(0.0f, 0.0f, -0.2f), c->closestLocationWS));
								debug_no_context();
#endif

								if (confussionDuration > 0.0f)
								{
									if (auto* aiObj = imo->get_as_ai_object())
									{
										if (auto* message = pilgrimOwner->get_in_world()->create_ai_message(NAME(aimConfussion)))
										{
											message->access_param(NAME(confussionDuration)).access_as<float>() = confussionDuration;
											message->to_ai_object(cast_to_nonconst(aiObj));
										}
									}
								}
							}
						}

						if (auto* s = owner->get_sound())
						{
							s->play_sound(NAME(push));
						}
					}
				}
			}
		}
	}
}
