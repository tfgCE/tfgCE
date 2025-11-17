#include "moduleExplosion.h"

#include "moduleEnergyQuantum.h"
#include "moduleEquipment.h"
#include "moduleOrbItem.h"
#include "modulePilgrim.h"

#include "..\shield.h"
#include "..\custom\mc_pickup.h"
#include "..\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\world\presenceLink.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

// !@#
#include "..\..\library\library.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\core\debug\extendedDebug.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define MEASURE_TO_TIMES
#endif

//

using namespace TeaForGodEmperor;

//

// params
DEFINE_STATIC_NAME(explosionCollisionFlags);
DEFINE_STATIC_NAME(range);
DEFINE_STATIC_NAME(impulseMassBase);
DEFINE_STATIC_NAME(impulseSpeed);
DEFINE_STATIC_NAME(impulseMassCoefRange);
DEFINE_STATIC_NAME(cantKillPilgrim);
DEFINE_STATIC_NAME(confussionDuration);

// message
DEFINE_STATIC_NAME_STR(aimConfussion, TXT("confussion"));

//

REGISTER_FOR_FAST_CAST(ModuleExplosion);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleExplosion(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleExplosionData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleExplosion::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("explosion")), create_module, create_module_data);
}

ModuleExplosion::ModuleExplosion(Framework::IModulesOwner* _owner)
: base( _owner )
{
	SET_EXTRA_DEBUG_INFO(forcedFullDamageFor, TXT("ModuleExplosion.forcedFullDamageFor"));
}

ModuleExplosion::~ModuleExplosion()
{
}

void ModuleExplosion::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	moduleExplosionData = fast_cast<ModuleExplosionData>(_moduleData);

	explosionCollisionFlags = _moduleData->get_parameter<String>(this, NAME(explosionCollisionFlags), explosionCollisionFlags);
	if (!explosionCollisionFlags.is_empty())
	{
		useExplosionCollisionFlags = Collision::Flags::none();
		useExplosionCollisionFlags.access().apply(explosionCollisionFlags);
	}
}

void ModuleExplosion::reset()
{
	base::reset();

	forcedFullDamageFor.clear();
}

void ModuleExplosion::activate()
{
	base::activate();

	range = Range(2.0f); // min is below which whole damage is taken

	todo_hack(TXT("if we do this in initialisation, we may be not able to get values properly, not for temporary object. we need to do full setup as otherwise if something is missing it will leak to other objects"));
	damage.setup_with(this, moduleExplosionData);
	if (!damage.explosionDamage)
	{
		damage.explosionDamage = true;
		warn(TXT("explosion damage not set for \"%S\", automatically setting"), moduleExplosionData->get_in_library_stored()? moduleExplosionData->get_in_library_stored()->get_name().to_string().to_char() : TXT("--"));
	}
	continuousDamage.setup_with(this, &damage, moduleExplosionData);
	if (continuousDamage.is_valid() &&
		!continuousDamage.explosionDamage)
	{
		continuousDamage.explosionDamage = true;
		warn(TXT("explosion damage not set for \"%S\", automatically setting"), moduleExplosionData->get_in_library_stored() ? moduleExplosionData->get_in_library_stored()->get_name().to_string().to_char() : TXT("--"));
	}

	range = moduleExplosionData->range;
	range = moduleExplosionData->get_parameter<Range>(this, NAME(range), range);
	impulseMassBase = moduleExplosionData->get_parameter<float>(this, NAME(impulseMassBase), impulseMassBase);
	impulseSpeed = moduleExplosionData->get_parameter<float>(this, NAME(impulseSpeed), impulseSpeed);
	impulseMassCoefRange = moduleExplosionData->get_parameter<Range>(this, NAME(impulseMassCoefRange), impulseMassCoefRange);
	cantKillPilgrim = moduleExplosionData->get_parameter<bool>(this, NAME(cantKillPilgrim), cantKillPilgrim);
	confussionDuration = moduleExplosionData->get_parameter<float>(this, NAME(confussionDuration), confussionDuration);

	doExplosion = true;
}

void ModuleExplosion::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	if (doExplosion)
	{
		MEASURE_AND_OUTPUT_PERFORMANCE(TXT("explode"));
		do_damage(get_owner()->get_presence()->get_placement().get_translation(), get_owner()->get_presence()->get_in_room());
		doExplosion = false;
	}
}

void ModuleExplosion::force_full_damage_for(Framework::IModulesOwner* _imo)
{
	MODULE_OWNER_LOCK(TXT("ModuleExplosion::force_full_damage_for"));

	forcedFullDamageFor.push_back(SafePtr<Framework::IModulesOwner>(_imo));
}

void ModuleExplosion::collect_to_damage(Vector3 loc, Framework::Room* inRoom, float distanceLeft, ArrayStack<CollectedToDamage> & collected, Framework::DoorInRoom* throughDoor, Transform const & roomTransform)
{
	if (!inRoom)
	{
		return;
	}
	Vector3 locInOtherRoom = roomTransform.location_to_local(loc);
	int collectedCount = 0;
	FOR_EVERY_OBJECT_IN_ROOM(obj, inRoom) // it may be called at any time, also when we're able to damage/remove objects
	{
		if (auto* p = obj->get_custom<CustomModules::Pickup>())
		{
			if (p->is_in_pocket() || p->is_in_holder())
			{
				// objects in pockets are safe
				continue;
			}
		}
		if (auto* h = obj->get_custom<CustomModules::Health>())
		{
			if (h->is_invincible())
			{
				// completely ignore such objects
				continue;
			}
			if (auto * op = obj->get_presence())
			{
				Vector3 oLoc = op->get_centre_of_presence_transform_WS().get_translation();

				if (throughDoor && throughDoor->get_plane().get_in_front(oLoc) < 0.0f)
				{
					// skip it, behind the door
					continue;
				}

				CollectedToDamage ctd;
				ctd.object = obj;
				ctd.inRoom = inRoom;
				ctd.health = h;
				ctd.locationWS = roomTransform.location_to_world(oLoc);
				ctd.placementWS = roomTransform.to_world(op->get_placement());
				ctd.hitRelativeDir = op->get_centre_of_presence_os().vector_to_local(op->get_placement().vector_to_local(oLoc - locInOtherRoom)).normal();
				debug_context(inRoom);
				debug_push_transform(op->get_placement());
				debug_push_transform(op->get_centre_of_presence_os());
				debug_draw_time_based(10000.0f, debug_draw_arrow(true, Colour::red, Vector3::zero, ctd.hitRelativeDir));
				debug_draw_time_based(10000.0f, debug_draw_text(true, Colour::red, Vector3::zero, NP, NP, NP, false, obj->ai_get_name().to_char()));
				debug_pop_transform();
				debug_pop_transform();
				debug_no_context();
				collectedCount++;
				if (collectedCount > collected.get_max_size())
				{
					warn(TXT("circular collect to damage? %i v %i"), collectedCount, collected.get_max_size());
				}
				collected.push_back_or_replace(ctd); // better to fail here but how is it possible that we exceed this amount? 
				if (! collected.has_place_left())
				{
					break;
				}
			}
		}
	}

	for_every_ptr(door, inRoom->get_doors())
	{
		if (door != throughDoor &&
			door->is_visible() &&
			door->has_world_active_room_on_other_side())
		{
			Plane plane = door->get_plane();
			if (plane.get_in_front(loc) > 0.0f)
			{
				float dist = door->calculate_dist_of(loc);
				if (dist < distanceLeft)
				{
					collect_to_damage(door->get_other_room_transform().location_to_local(loc), door->get_world_active_room_on_other_side(), distanceLeft - dist, collected, door->get_door_on_other_side(), roomTransform.to_world(door->get_other_room_transform()));
				}
			}
		}
	}
}

void ModuleExplosion::do_damage(Vector3 loc, Framework::Room* inRoom)
{
#ifdef MEASURE_TO_TIMES
	MEASURE_AND_OUTPUT_PERFORMANCE(TXT("explosion_do_damage"));
#endif

	debug_filter(explosions);
	ARRAY_STACK(CollectedToDamage, collected, 128);
	{
#ifdef MEASURE_TO_TIMES
		MEASURE_AND_OUTPUT_PERFORMANCE(TXT("collect to damage"));
#endif
		collect_to_damage(loc, inRoom, range.max, collected);
	}

#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(explosions)
		{
			output(TXT("EXPLOSION do damage"));
		}
	}
#endif

	if (auto* presence = get_owner()->get_presence())
	{
#ifdef MEASURE_TO_TIMES
		MEASURE_AND_OUTPUT_PERFORMANCE(TXT("trace to damage"));
#endif
		if (useExplosionCollisionFlags.is_set())
		{
			// check if we have trace to object to damage
			for (int i = 0; i < collected.get_size(); ++i)
			{
				auto & ctd = collected[i];
				if (ctd.forceDamage)
				{
#ifdef AN_ALLOW_EXTENDED_DEBUG
					IF_EXTENDED_DEBUG(explosions)
					{
						output(TXT(" [%i] %S forced"), i, ctd.object->ai_get_name().to_char());
					}
#endif
					continue;
				}

#ifdef MEASURE_TO_TIMES
				MEASURE_AND_OUTPUT_PERFORMANCE(TXT("trace ctd %i"), i);
#endif

				auto* hitPilgrim = ctd.object->get_gameplay_as<ModulePilgrim>();
				bool allowToOcclude = true;
				if (allowToOcclude && hitPilgrim)
				{
					float best = 0.5f;
					Framework::IModulesOwner* replaceWith = nullptr;
					Vector3 replaceLocationWS = Vector3::zero;
					Transform replacePlacementWS = Transform::identity;
					Vector3 replaceHitRelativeDir = Vector3::zero;
					for (int i = 0; i < Hand::MAX; ++i)
					{
						Transform hitPilgrimPlacement = ctd.placementWS;
						if (auto * hand = hitPilgrim->get_hand(Hand::Type(i)))
						{
							Concurrency::ScopedSpinLock lock(hand->get_presence()->access_attached_lock());
							for_every_ptr(attached, hand->get_presence()->get_attached())
							{
								if (auto * shield = attached->get_gameplay_as<IShield>())
								{
									Transform shieldPlacementWS;
									Framework::Room* inRoom;
									attached->get_presence()->get_attached_placement_for_top(OUT_ inRoom, OUT_ shieldPlacementWS, hitPilgrim->get_owner());
									{
										Vector3 shieldCoverDirOS = shield->get_cover_dir_os();
										Vector3 shieldCoverDirWS = shieldPlacementWS.vector_to_world(shieldCoverDirOS);
										Vector3 hitLocationWS = shieldPlacementWS.location_to_world(attached->get_presence()->get_centre_of_presence_os().get_translation());
										float dotResult = Vector3::dot((loc - hitLocationWS).normal(), shieldCoverDirWS.normal());
										debug_context(presence->get_in_room());
										debug_draw_time_based(10000.0f, debug_draw_arrow(true, Colour::green, shieldPlacementWS.get_translation(), shieldPlacementWS.get_translation() + shieldCoverDirWS));
										debug_draw_time_based(10000.0f, debug_draw_arrow(true, Colour::red, shieldPlacementWS.get_translation(), shieldPlacementWS.get_translation() + (hitLocationWS - loc).normal()));
										debug_draw_time_based(10000.0f, debug_draw_text(true, Colour::orange, shieldPlacementWS.get_translation(), NP, NP, NP, false, String::printf(TXT("%.3f"), dotResult).to_char()));
										debug_pop_transform();
										debug_pop_transform();
										debug_no_context();
										if (dotResult > best)
										{
											best = dotResult;
											replaceWith = attached;
											replacePlacementWS = shieldPlacementWS;
											replaceLocationWS = shieldPlacementWS.location_to_world(attached->get_presence()->get_centre_of_presence_os().get_translation());
											replaceHitRelativeDir = -shieldPlacementWS.location_to_local(loc).normal(); // not used?
										}
									}
								}
							}
						}
					}
					if (replaceWith)
					{
						ctd.occluded = true;
						allowToOcclude = false;

						bool alreadyExists = false;
						for_every(ctd, collected)
						{
							if (ctd->object == replaceWith)
							{
								ctd->forceDamage = true;
								alreadyExists = true;
								break;
							}
						}
						if (!alreadyExists)
						{
							CollectedToDamage newCtd;
							newCtd.object = replaceWith;
							newCtd.inRoom = replaceWith->get_presence()->get_in_room();
							newCtd.health = replaceWith->get_custom<CustomModules::Health>(); // might be null
							newCtd.locationWS = replaceLocationWS;
							newCtd.placementWS = replacePlacementWS;
							newCtd.hitRelativeDir = replaceHitRelativeDir;
							newCtd.forceDamage = true;
							collected.push_back_if_possible(newCtd);
							if (!collected.has_place_left())
							{
								break;
							}
						}
					}
				}

				if (allowToOcclude)
				{
					Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);

					collisionTrace.add_location(loc);
					collisionTrace.add_location(ctd.locationWS);

					Framework::CheckSegmentResult result;
					int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom
											| Framework::CollisionTraceFlag::ProvideNonHitResult; // we want to make sure we're in same room! 
					Framework::CheckCollisionContext checkCollisionContext;
					checkCollisionContext.collision_info_not_needed();
					checkCollisionContext.use_collision_flags(useExplosionCollisionFlags.get());

					if (presence->trace_collision(AgainstCollision::Precise, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
					{
						if (auto* hitObject = fast_cast<Framework::IModulesOwner>(result.object))
						{
							if (hitObject != ctd.object)
							{
								ctd.occluded = true;
								allowToOcclude = false;
#ifdef AN_ALLOW_EXTENDED_DEBUG
								IF_EXTENDED_DEBUG(explosions)
								{
									output(TXT("EXPLOSION %S occluded by %S"), ctd.object->ai_get_name().to_char(), hitObject->ai_get_name().to_char());
								}
#endif
								if (auto* h = hitObject->get_custom<CustomModules::Health>())
								{
									if (result.presenceLink)
									{
										bool alreadyExists = false;
										for_every(ctd, collected)
										{
											if (ctd->object == hitObject)
											{
												ctd->forceDamage = true;
												alreadyExists = true;
												break;
											}
										}
										debug_context(result.inRoom);
										debug_push_transform(result.presenceLink->get_placement_in_room());
										debug_push_transform(result.presenceLink->get_modules_owner()->get_presence()->get_centre_of_presence_os());
										debug_draw_time_based(10000.0f, debug_draw_arrow(true, Colour::orange, Vector3::zero, -(result.presenceLink->get_placement_in_room().location_to_local(result.intoInRoomTransform.location_to_world(loc)) - result.presenceLink->get_modules_owner()->get_presence()->get_centre_of_presence_os().get_translation()).normal()));
										debug_draw_time_based(10000.0f, debug_draw_text(true, Colour::orange, Vector3::zero, NP, NP, NP, false, hitObject->ai_get_name().to_char()));
										debug_pop_transform();
										debug_pop_transform();
										debug_no_context();
										if (!alreadyExists)
										{
											CollectedToDamage newCtd;
											newCtd.object = hitObject;
											newCtd.inRoom = result.inRoom;
											newCtd.health = h;
											newCtd.locationWS = result.intoInRoomTransform.location_to_world(result.hitLocation);
											an_assert(result.presenceLink);
											newCtd.placementWS = result.presenceLink ? result.intoInRoomTransform.to_world(result.presenceLink->get_placement_in_room()) : Transform(newCtd.locationWS, Quat::identity);
											newCtd.hitRelativeDir = -(result.presenceLink->get_placement_in_room().location_to_local(result.intoInRoomTransform.location_to_world(loc)) - result.presenceLink->get_modules_owner()->get_presence()->get_centre_of_presence_os().get_translation()).normal();
											newCtd.forceDamage = true;
											collected.push_back_if_possible(newCtd);
										}
									}
								}
							}
						}
						else
						{
#ifdef AN_ALLOW_EXTENDED_DEBUG
							IF_EXTENDED_DEBUG(explosions)
							{
								output(TXT("EXPLOSION %S occluded"), ctd.object->ai_get_name().to_char());
							}
#endif
							ctd.occluded = true;
						}
					}
					else
					{
						if (ctd.inRoom != result.inRoom)
						{
#ifdef AN_ALLOW_EXTENDED_DEBUG
							IF_EXTENDED_DEBUG(explosions)
							{
								output(TXT("EXPLOSION %S in different room "), ctd.object->ai_get_name().to_char());
							}
#endif
							// in different room
							ctd.occluded = true;
						}
					}
				}
			}
		}
	}
#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(explosions)
		{
			output(TXT("EXPLOSION --- do damage"));
		}
	}
#endif

	{
#ifdef MEASURE_TO_TIMES
		MEASURE_AND_OUTPUT_PERFORMANCE(TXT("do damage"));
#endif
		for_every(ctd, collected)
		{
			if (!ctd->forceDamage && ctd->occluded)
			{
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(explosions)
				{
					output(TXT("EXPLOSION skip %S"), ctd->object->ai_get_name().to_char());
				}
#endif
				continue;
			}
#ifdef MEASURE_TO_TIMES
			MEASURE_AND_OUTPUT_PERFORMANCE(TXT("do damage ctd %i"), for_everys_index(ctd));
#endif
			float dist = 0.0f;
			if (auto* c = ctd->object->get_collision())
			{
				dist = c->calc_distance_from_primitive_os(ctd->placementWS.location_to_local(loc), false);
			}
			else
			{
				dist = (ctd->locationWS - loc).length();
			}
			{
				bool forcedFullDamage = false;
				for_every_ref(f, forcedFullDamageFor)
				{
					if (f == ctd->object)
					{
						forcedFullDamage = true;
					}
				}
				if (forcedFullDamage)
				{
					dist = 0.0f; // max!
				}
			}
			if (dist <= range.max)
			{
				float affect = clamp(1.0f - range.get_pt_from_value(dist), 0.0f, 1.0f);
				if (affect > 0.0f)
				{
					Damage damageNow = damage;
					damageNow.damage = damageNow.damage.mul(affect);

					DamageInfo damageInfo;
					damageInfo.damager = get_owner();
					damageInfo.source = damageInfo.damager;
					damageInfo.instigator = get_owner()->get_valid_top_instigator();
					damageInfo.hitRelativeDir = ctd->hitRelativeDir;
					damageInfo.cantKillPilgrim = cantKillPilgrim;

#ifdef AN_ALLOW_EXTENDED_DEBUG
					IF_EXTENDED_DEBUG(explosions)
					{
						output(TXT("EXPLOSION damage %S (dist:%.3f, damage:%.3f)"), ctd->object->ai_get_name().to_char(), dist, damageNow.damage);
					}
#endif

					if (ctd->health)
					{
						ctd->health->adjust_damage_on_hit_with_extra_effects(REF_ damageNow); 
						ctd->health->deal_damage(damageNow, damageNow, damageInfo);

						if (continuousDamage.is_valid())
						{
							ctd->health->add_continuous_damage(continuousDamage, damageInfo, affect);
						}
					}
					if (confussionDuration > 0.0f)
					{
						if (auto* aiObj = ctd->object->get_as_ai_object())
						{
							if (auto* message = get_owner()->get_in_world()->create_ai_message(NAME(aimConfussion)))
							{
								message->access_param(NAME(confussionDuration)).access_as<float>() = confussionDuration;
								message->to_ai_object(cast_to_nonconst(aiObj));
							}
						}
					}
					if (auto* p = ctd->object->get_presence())
					{
						float massCoef = 1.0f;
						if (auto* c = ctd->object->get_collision())
						{
							massCoef = impulseMassBase / c->get_mass();
						}
						float speed = impulseSpeed * impulseMassCoefRange.clamp(massCoef);
						//if (auto* c = ctd->object->get_collision())
						//{
						//	speed = speed * clamp(100.0f / c->get_mass(), 0.25f, 4.0f);
						//}
						if (ctd->object->get_gameplay_as<ModuleEnergyQuantum>() ||
							ctd->object->get_gameplay_as<ModuleOrbItem>())
						{
							// don't move energy quantums that much when exploding
							affect *= 0.05f;
						}
						p->add_velocity_impulse(p->get_placement().vector_to_world(damageInfo.hitRelativeDir.get()) * affect * speed);
					}
				}
			}
		}
	}
	debug_no_filter();
}

//

REGISTER_FOR_FAST_CAST(ModuleExplosionData);

ModuleExplosionData::ModuleExplosionData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleExplosionData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);
	
	return result;
}

bool ModuleExplosionData::read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("range"))
	{
		range.load_from_string(_attr->get_as_string());
		return true;
	}
	return base::read_parameter_from(_attr, _lc);
}

bool ModuleExplosionData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
