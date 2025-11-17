#include "aiLogic_exmDeflector.h"

#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\moduleProjectile.h"

#include "..\..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(deflectionAcceleration);
DEFINE_STATIC_NAME(deflectionSafeRadius);
DEFINE_STATIC_NAME(deflectionMaxDistance);

// sound / temporary objects
DEFINE_STATIC_NAME(deflect);
DEFINE_STATIC_NAME(idle);

//

REGISTER_FOR_FAST_CAST(EXMDeflector);

EXMDeflector::EXMDeflector(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMDeflector::~EXMDeflector()
{
}

void EXMDeflector::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
	acceleration = _parameters.get_value<float>(NAME(deflectionAcceleration), acceleration);
	safeRadius = _parameters.get_value<float>(NAME(deflectionSafeRadius), safeRadius);
	maxDistance = _parameters.get_value<float>(NAME(deflectionMaxDistance), maxDistance);
}

void EXMDeflector::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	bool shouldPlayDeflect = false;
	bool shouldPlayIdle = false; // means that deflect works

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX)
	{
		if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			exmModule->mark_exm_active(!exmModule->is_exm_active());
		}
		if (exmModule->is_exm_active())
		{
			shouldPlayIdle = true;
			if (pilgrimOwner->get_presence() &&
				pilgrimOwner->get_presence()->get_in_room())
			{
				Vector3 pilgrimCentreWS = pilgrimOwner->get_presence()->get_centre_of_presence_WS();
				ARRAY_STACK(Framework::FoundRoom, rooms, 16);
				Framework::FoundRoom::find(rooms, pilgrimOwner->get_presence()->get_in_room(), pilgrimCentreWS,
					Framework::FindRoomContext().only_visible().with_depth_limit(6).with_max_distance(maxDistance));
				for_every(foundRoom, rooms)
				{
					Transform foundRoomTransform = Transform::identity;
					auto* goUp = foundRoom;
					while (goUp && goUp->enteredThroughDoor)
					{
						foundRoomTransform = goUp->enteredThroughDoor->get_door_on_other_side()->get_other_room_transform().to_world(foundRoomTransform);
						goUp = goUp->enteredFrom;
					}
					//FOR_EVERY_TEMPORARY_OBJECT_IN_ROOM(to, foundRoom->room)
					for_every_ptr(to, foundRoom->room->get_temporary_objects())
					{
						if (auto * mp = to->get_gameplay_as<ModuleProjectile>())
						{
							auto* top = to->get_presence();
							if (top && mp->is_deflectable())
							{
								Vector3 locWS = foundRoomTransform.location_to_world(top->get_placement().get_translation());
								//debug_push_filter(alwaysDraw);
								//debug_context(pilgrimOwner->get_presence()->get_in_room());
								//debug_draw_line(true, Colour::green, pilgrimCentreWS, locWS);
								//debug_no_context();
								//debug_pop_filter();
								if ((locWS - pilgrimCentreWS).length_squared() < sqr(maxDistance))
								{
									Vector3 velWS = foundRoomTransform.vector_to_world(top->get_velocity_linear());
									Vector3 towardsProjectileOSXY = pilgrimOwner->get_presence()->get_placement().vector_to_local(locWS - pilgrimCentreWS).normal_2d();
									if (!towardsProjectileOSXY.is_zero())
									{
										Plane planeTowardsProjectileOSXY = Plane(towardsProjectileOSXY, Vector3::zero);
										Vector3 endLocWS = locWS + velWS.normal() * 1000.0f;
										Segment projectileSegmentOS(pilgrimOwner->get_presence()->get_placement().location_to_local(locWS), pilgrimOwner->get_presence()->get_placement().location_to_local(endLocWS));
										float t = planeTowardsProjectileOSXY.calc_intersection_t(projectileSegmentOS);
										if (t >= 0.0f && t <= 1.0f)
										{
											Vector3 hitAtOS = projectileSegmentOS.get_at_t(t);
											float atDistance = pilgrimOwner->get_collision()->calc_distance_from_primitive_os(hitAtOS, false);
											if (atDistance < safeRadius)
											{
												shouldPlayDeflect = true;
												Vector3 deflectionWS = pilgrimOwner->get_presence()->get_placement().vector_to_world(hitAtOS.normal_2d());
												Vector3 deflectionOWS = foundRoomTransform.vector_to_local(deflectionWS); // back to original one
												top->add_velocity_impulse(deflectionOWS * acceleration * _deltaTime * (1.0f - mp->get_anti_deflection()));
												//debug_push_filter(alwaysDraw);
												//debug_context(top->get_in_room());
												//debug_draw_line(true, Colour::red, top->get_placement().get_translation(), top->get_placement().get_translation() + deflectionOWS);
												//debug_no_context();
												//debug_pop_filter();
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (shouldPlayDeflect)
	{
		playDeflectFor = 0.5f;
	}
	else
	{
		playDeflectFor = max(0.0f, playDeflectFor - _deltaTime);
	}
	if (playDeflectFor > 0.0f && !isPlayingDeflect)
	{
		if (auto * s = owner->get_sound())
		{
			s->play_sound(NAME(deflect));
		}
		isPlayingDeflect = true;
	}
	if (playDeflectFor == 0.0f && isPlayingDeflect)
	{
		if (auto * s = owner->get_sound())
		{
			s->stop_sound(NAME(deflect));
		}
		isPlayingDeflect = false;
	}

	if (isPlayingDeflect)
	{
		shouldPlayIdle = false;
	}

	if (shouldPlayIdle)
	{
		if (!deflectParticles.get())
		{
			// spawn particles from here but attach to the pilgrim
			if (auto* pimo = pilgrimOwner.get())
			{
				if (auto* imo = owner.get())
				{
					if (auto* to = imo->get_temporary_objects())
					{
						deflectParticles = to->spawn_attached_to(NAME(idle), pimo);
						an_assert(deflectParticles.get());
					}
				}
			}
		}
	}
	else
	{
		if (auto* to = deflectParticles.get())
		{
			Framework::ParticlesUtils::desire_to_deactivate(to);
			deflectParticles.clear();
		}
	}


	/*
	if (shouldPlayIdle != isPlayingIdle)
	{
		isPlayingIdle = shouldPlayIdle;
		if (auto * s = owner->get_sound())
		{
			if (isPlayingIdle)
			{
				s->play_sound(NAME(idle));
			}
			else
			{
				s->stop_sound(NAME(idle));
			}
		}
	}
	*/
}
