#include "aiLogic_exmCorrosionCatalyst.h"

#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\moduleProjectile.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\room.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

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
DEFINE_STATIC_NAME(catalyseMaxDistanceInVisibleRoom);
DEFINE_STATIC_NAME(catalyseMaxDistanceBeyondVisibleRoom);
DEFINE_STATIC_NAME(catalyseDamage);
DEFINE_STATIC_NAME(catalyseDamageCoef);
DEFINE_STATIC_NAME(deflectionMaxDistance);

// sound
DEFINE_STATIC_NAME(catalyse);

// object tags
DEFINE_STATIC_NAME(corrosionLeak);

//

REGISTER_FOR_FAST_CAST(EXMCorrosionCatalyst);

EXMCorrosionCatalyst::EXMCorrosionCatalyst(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMCorrosionCatalyst::~EXMCorrosionCatalyst()
{
}

void EXMCorrosionCatalyst::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	maxDistanceInVisibleRoom = _parameters.get_value<float>(NAME(catalyseMaxDistanceInVisibleRoom), maxDistanceInVisibleRoom);
	maxDistanceBeyondVisibleRoom = _parameters.get_value<float>(NAME(catalyseMaxDistanceBeyondVisibleRoom), maxDistanceBeyondVisibleRoom);
	catalyseDamage = Energy::get_from_storage(_parameters, NAME(catalyseDamage), catalyseDamage);
	catalyseDamageCoef = EnergyCoef::get_from_storage(_parameters, NAME(catalyseDamageCoef), catalyseDamageCoef);
}

void EXMCorrosionCatalyst::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX)
	{
		if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			bool catalyse = exmModule->mark_exm_active_blink();

			if (catalyse)
			{
				if (pilgrimOwner->get_presence() &&
					pilgrimOwner->get_presence()->get_in_room())
				{
					SafePtr<Framework::IModulesOwner> safePilgrim(pilgrimOwner);
					Vector3 pilgrimCentreWS = pilgrimOwner->get_presence()->get_centre_of_presence_WS();
					ARRAY_STACK(Framework::FoundRoom, rooms, 16);
					Framework::FoundRoom::find(rooms, pilgrimOwner->get_presence()->get_in_room(), pilgrimCentreWS,
						Framework::FindRoomContext().with_depth_limit(6).with_max_distance(maxDistanceInVisibleRoom).with_max_distance_not_visible(maxDistanceBeyondVisibleRoom));
					for_every(foundRoom, rooms)
					{
						Transform foundRoomTransform = Transform::identity;
						auto* goUp = foundRoom;
						while (goUp && goUp->enteredThroughDoor)
						{
							foundRoomTransform = goUp->enteredThroughDoor->get_door_on_other_side()->get_other_room_transform().to_world(foundRoomTransform);
							goUp = goUp->enteredFrom;
						}
						//FOR_EVERY_OBJECT_IN_ROOM(object, foundRoom->room)
						for_every_ptr(object, foundRoom->room->get_objects())
						{
							if (auto* h = object->get_custom<CustomModules::Health>())
							{
								Energy useCatalyseDamage = catalyseDamage;
								EnergyCoef useCatalyseDamageCoef = catalyseDamageCoef;
								object->set_timer(Random::get_float(0.001f, 0.2f), 
									[safePilgrim, useCatalyseDamage, useCatalyseDamageCoef](Framework::IModulesOwner* _imo)
									{
										if (auto* h = _imo->get_custom<CustomModules::Health>())
										{
											auto* ob = _imo->get_as_object();
											if (ob && ob->get_tags().get_tag(NAME(corrosionLeak)))
											{
												Damage dealDamage;
												dealDamage.damage = h->get_health();
												dealDamage.damageType = DamageType::Gameplay;

												DamageInfo damageInfo;
												damageInfo.damager = safePilgrim.get();
												damageInfo.source = safePilgrim.get();
												damageInfo.instigator = safePilgrim.get();
												// don't adjust_damage_on_hit_with_extra_effects
												h->deal_damage(dealDamage, dealDamage, damageInfo);
											}
											else
											{
												h->apply_continuous_damage_instantly(DamageType::Corrosion, useCatalyseDamage, useCatalyseDamageCoef, safePilgrim.get());
											}
										}
									});
							}
						}
						//FOR_EVERY_TEMPORARY_OBJECT_IN_ROOM(to, foundRoom->room)
						for_every_ptr(to, foundRoom->room->get_temporary_objects())
						{
							if (auto* p = to->get_gameplay_as<ModuleProjectile>())
							{
								if (p->access_damage().damageType == DamageType::Corrosion)
								{
									p->force_abnormal_end(pilgrimOwner.get());
								}
							}
						}
					}
				}
				if (auto* s = owner->get_sound())
				{
					s->play_sound(NAME(catalyse));
				}
			}
		}
	}
}
