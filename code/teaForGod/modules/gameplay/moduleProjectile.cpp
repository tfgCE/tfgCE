#include "moduleProjectile.h"

#include "moduleEXM.h"
#include "moduleExplosion.h"
#include "modulePilgrim.h"
#include "modulePilgrimHand.h"

#include "..\custom\health\mc_health.h"

#include "..\..\library\physicalMaterial.h"
#include "..\..\utils.h"

#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleAppearanceImpl.inl"
#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\moduleMovementProjectile.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\presenceLink.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

// !@#
#include "..\..\library\library.h"
#include "..\..\..\framework\object\temporaryObject.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\profilePerformanceSettings.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#define INSPECT_DAMAGE
	#define INSPECT_PROJECTILE_LIFETIME
	#define INSPECT_PROJECTILE_DAMAGE
	//#define DRAW_INSPECT_PROJECTILE_LIFETIME
	#define LOG_PROJECTILE_SPAWN_ON_HIT
#endif

//

using namespace TeaForGodEmperor;

//

// message
DEFINE_STATIC_NAME(projectileHit);

// message params
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(instigator);
DEFINE_STATIC_NAME(hit);
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRoomLocation);
DEFINE_STATIC_NAME(inRoomNormal);
DEFINE_STATIC_NAME(projectile);

// module params
DEFINE_STATIC_NAME(deflectable);
DEFINE_STATIC_NAME(ricochetLimit);
DEFINE_STATIC_NAME(autoGetEmissiveColours);
DEFINE_STATIC_NAME(dontAutoGetEmissiveColours);
DEFINE_STATIC_NAME(spawnPhysicalMaterialProjectiles);
DEFINE_STATIC_NAME(dontSpawnPhysicalMaterialProjectiles);
DEFINE_STATIC_NAME(speedToCeaseExist);
DEFINE_STATIC_NAME(ceaseOnHit);
DEFINE_STATIC_NAME(ceaseOnDealDamage);
DEFINE_STATIC_NAME(dealDamageOnHit);
DEFINE_STATIC_NAME(explodeOnHit);
DEFINE_STATIC_NAME(explodeOnHitHealth);
DEFINE_STATIC_NAME(explodeOnDeath);
DEFINE_STATIC_NAME(explodeOnCollisionWithFlags);
DEFINE_STATIC_NAME(explosionDamageCoef);
DEFINE_STATIC_NAME(explosionContinuousDamageCoef);
DEFINE_STATIC_NAME(explosionRange);
DEFINE_STATIC_NAME(redistributeEnergyOnExplosion);

// object variables
DEFINE_STATIC_NAME(projectileEmissiveColour);
DEFINE_STATIC_NAME(projectileEmissiveBaseColour);
DEFINE_STATIC_NAME(projectileEnergyAbsorberCoef);

// appearance uniforms
DEFINE_STATIC_NAME(projectileSpeed);
DEFINE_STATIC_NAME(projectileDamage);
DEFINE_STATIC_NAME(projectileTravelledDistance);
DEFINE_STATIC_NAME(projectileTravelledDistanceActive);
DEFINE_STATIC_NAME(emissiveColour); // we use emissive directly, without emissive controller!
DEFINE_STATIC_NAME(emissiveBaseColour);

// temporary objects
DEFINE_STATIC_NAME(explode);
DEFINE_STATIC_NAME_STR(explodePlasma, TXT("explode_plasma"));
DEFINE_STATIC_NAME_STR(explodeIon, TXT("explode_ion"));
DEFINE_STATIC_NAME_STR(explodeCorrosion, TXT("explode_corrosion"));
DEFINE_STATIC_NAME_STR(explodeLightning, TXT("explode_lightning"));

// sound
DEFINE_STATIC_NAME(absorbed);

//

REGISTER_FOR_FAST_CAST(ModuleProjectile);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleProjectile(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleProjectileData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleProjectile::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("projectile")), create_module, create_module_data);
}

ModuleProjectile::ModuleProjectile(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModuleProjectile::~ModuleProjectile()
{
}

void ModuleProjectile::reset()
{
	// clear run variables
	exploded = false;
	ricocheted = 0;
	projectileEmissiveColour = Colour::none;
	projectileEmissiveBaseColour = Colour::none;
	get_owner()->access_variables().access<Colour>(NAME(projectileEmissiveColour)) = projectileEmissiveColour;
	get_owner()->access_variables().access<Colour>(NAME(projectileEmissiveBaseColour)) = projectileEmissiveBaseColour;

	base::reset();
}

void ModuleProjectile::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	moduleProjectileData = fast_cast<ModuleProjectileData>(_moduleData);

	autoGetEmissiveColours = moduleProjectileData->get_parameter<bool>(this, NAME(autoGetEmissiveColours), autoGetEmissiveColours);
	autoGetEmissiveColours = !moduleProjectileData->get_parameter<bool>(this, NAME(dontAutoGetEmissiveColours), ! autoGetEmissiveColours);

	spawnPhysicalMaterialProjectiles = moduleProjectileData->get_parameter<bool>(this, NAME(spawnPhysicalMaterialProjectiles), spawnPhysicalMaterialProjectiles);
	spawnPhysicalMaterialProjectiles = !moduleProjectileData->get_parameter<bool>(this, NAME(dontSpawnPhysicalMaterialProjectiles), !spawnPhysicalMaterialProjectiles);

	speedToCeaseExist = moduleProjectileData->get_parameter<float>(this, NAME(speedToCeaseExist), speedToCeaseExist);

	damage.setup_with(this, moduleProjectileData);
	continuousDamage.setup_with(this, nullptr, moduleProjectileData);

	isDeflectable = moduleProjectileData->get_parameter<bool>(this, NAME(deflectable), isDeflectable);
	ricochetLimit = moduleProjectileData->get_parameter<int>(this, NAME(ricochetLimit), ricochetLimit);

	ceaseOnHit = moduleProjectileData->get_parameter<bool>(this, NAME(ceaseOnHit), ceaseOnHit);
	ceaseOnDealDamage = moduleProjectileData->get_parameter<bool>(this, NAME(ceaseOnDealDamage), ceaseOnDealDamage);
	dealDamageOnHit = moduleProjectileData->get_parameter<bool>(this, NAME(dealDamageOnHit), dealDamageOnHit);
	explodeOnHit = moduleProjectileData->get_parameter<bool>(this, NAME(explodeOnHit), explodeOnHit);
	explodeOnHitHealth = moduleProjectileData->get_parameter<bool>(this, NAME(explodeOnHitHealth), explodeOnHitHealth);
	explodeOnDeath = moduleProjectileData->get_parameter<bool>(this, NAME(explodeOnDeath), explodeOnDeath);
	explodeOnCollisionWithFlags.apply(moduleProjectileData->get_parameter<String>(this, NAME(explodeOnCollisionWithFlags)));
	explosionDamageCoef = EnergyCoef::get_from_module_data(this, moduleProjectileData, NAME(explosionDamageCoef), explosionDamageCoef);
	explosionContinuousDamageCoef = EnergyCoef::get_from_module_data(this, moduleProjectileData, NAME(explosionContinuousDamageCoef), explosionContinuousDamageCoef);
	explosionRange = moduleProjectileData->get_parameter<Range>(this, NAME(explosionRange), explosionRange);
}

void ModuleProjectile::activate()
{
	base::activate();

	ricocheted = 0;
	travelledDistance = 0.0f;
	travelledDistanceActive = 1.0f;

	// update from what we were set with
	if (autoGetEmissiveColours)
	{
		projectileEmissiveColour = get_owner()->get_variables().get_value<Colour>(NAME(projectileEmissiveColour), Colour::none);
		projectileEmissiveBaseColour = get_owner()->get_variables().get_value<Colour>(NAME(projectileEmissiveBaseColour), Colour::none);
	}
	else
	{
		projectileEmissiveColour = Colour::none;
		projectileEmissiveBaseColour = Colour::none;
	}

	// set it just one time
	update_appearance_variables();

	get_owner()->get_presence()->store_velocities(1); // store one velocity, in case we ricocheted
}

void ModuleProjectile::update_appearance_variables(int _flags)
{
	an_assert(get_owner()->get_appearances().get_size() == 1, TXT("only single appearance handled"));
	if (auto* ap = get_owner()->get_appearance())
	{
		if (is_flag_set(_flags, UAV_Speed))
		{
			ap->set_shader_param(NAME(projectileSpeed), get_owner()->get_presence()->get_velocity_linear().length());
		}
		if (is_flag_set(_flags, UAV_Damage))
		{
			ap->set_shader_param(NAME(projectileDamage), (damage.damage.as_float() + continuousDamage.damage.as_float()));
		}

		if (is_flag_set(_flags, UAV_Emissives) &&
			autoGetEmissiveColours)
		{
			Utils::get_projectile_emissives_from_appearance(ap, OUT_ projectileEmissiveColour, OUT_ projectileEmissiveBaseColour);
			Utils::calculate_projectile_emissive_colour_base(projectileEmissiveColour, projectileEmissiveBaseColour);

			set_emissive_to(get_owner());
		}
	}
}

void ModuleProjectile::set_emissive_to(Framework::IModulesOwner* _imo) const
{
	if (! projectileEmissiveColour.is_none())
	{
		an_assert(!projectileEmissiveBaseColour.is_none(), TXT("both or none should be set"));
		if (auto* ap = _imo->get_appearance())
		{
			// set projectile emissives to appearance
			ap->set_shader_param(NAME(emissiveColour), projectileEmissiveColour.to_vector4());
			ap->set_shader_param(NAME(emissiveBaseColour), projectileEmissiveBaseColour.to_vector4());
		}
	}
}

void ModuleProjectile::force_abnormal_end(Framework::IModulesOwner* _instigator)
{
#ifdef INSPECT_PROJECTILE_LIFETIME
	output(TXT("[projectile] to'%p force abnormal end"), get_owner());
#endif
	if (explodeOnHit)
	{
		explode();
	}
	else
	{
#ifdef INSPECT_PROJECTILE_LIFETIME
		output(TXT("[projectile] to'%p cease to exist (force abnormal end)"), get_owner());
#endif
		Framework::ParticlesUtils::desire_to_deactivate(get_owner());
		get_owner()->cease_to_exist(false); // this may be called via logic, we don't want to be removed from the world then
	}
}

bool ModuleProjectile::process_hit(Framework::CollisionInfo const & _ci)
{
#ifdef INSPECT_PROJECTILE_LIFETIME
	output(TXT("[projectile] to'%p process hit \"%S\""), get_owner(), _ci.presenceLink && _ci.presenceLink->get_modules_owner()? _ci.presenceLink->get_modules_owner()->ai_get_name().to_char() : TXT("--"));
#endif

#ifdef INSPECT_PROJECTILE_DAMAGE
	output(TXT("[projectile] to'%p hit \"%S\" damage %.2f"), get_owner(), _ci.presenceLink && _ci.presenceLink->get_modules_owner() ? _ci.presenceLink->get_modules_owner()->ai_get_name().to_char() : TXT("--"), damage.damage.as_float());
#endif

	if (explodeOnHit && ! isPenetrator)
	{
#ifdef INSPECT_PROJECTILE_DAMAGE
		output(TXT("[projectile] to'%p explode"), get_owner());
#endif
		explode(nullptr, NP, fast_cast<Framework::IModulesOwner>(_ci.collidedWithObject.get()));
		return true;
	}

	maxDamageAmount = max(maxDamageAmount, damage.damage);

	if (damage.damage.is_zero() &&
		! continuousDamage.is_valid())
	{
#ifdef INSPECT_PROJECTILE_DAMAGE
		output(TXT("[projectile] to'%p no damage"), get_owner());
#endif
		// not this one, there's nothing to process
		// this will lead to cease to exist and maybe this is what should happen
		return false;
	}

	auto* hitImo = fast_cast<Framework::IModulesOwner>(_ci.collidedWithObject.get());

#ifdef INSPECT_PROJECTILE_DAMAGE
	output(TXT("[projectile] to'%p process damage %.2f"), get_owner(), damage.damage.as_float());
#endif

	{
		auto* hi = hitImo;
		while (hi)
		{
			if (auto* health = hi->get_custom<CustomModules::Health>())
			{
				health->adjust_damage_on_hit_with_extra_effects(REF_ damage);
#ifdef INSPECT_PROJECTILE_DAMAGE
				output(TXT("[projectile] to'%p adjusted damage by \"%S\" is %.2f"), get_owner(), hi->ai_get_name().to_char(), damage.damage.as_float());
#endif
			}
			auto* prevHi = hi;
			hi = hi->get_presence() ? hi->get_presence()->get_attached_to() : nullptr;
			if (hi == prevHi || damage.damageOnlyToHitObject)
			{
				break;
			}
		}
	}

#ifdef INSPECT_PROJECTILE_DAMAGE
	output(TXT("[projectile] to'%p hit material \"%S\""), get_owner(), _ci.material? _ci.material->get_name().to_string().to_char() : TXT("--"));
	output(TXT("[projectile] to'%p projectile type \"%S\""), get_owner(), moduleProjectileData->projectileType.to_string().to_char());
#endif

	PhysicalMaterial::ForProjectile const * useForProjectile = PhysicalMaterial::get_for_projectile(_ci.material, damage, moduleProjectileData? moduleProjectileData->projectileType : Tags::none);
		
	DamageInfo damageInfo;

	damageInfo.damager = get_owner(); // projectile
	damageInfo.source = get_owner()->get_instigator(); // weapon that fired a projectile
	damageInfo.instigator = get_owner()->get_valid_top_instigator(); // someone that was using a weapon
	if (_ci.collidedWithShape && _ci.collidedWithShape->get_collidable_shape_bone().is_valid())
	{
		damageInfo.hitBone = _ci.collidedWithShape->get_collidable_shape_bone();
	}
	damageInfo.hitPhysicalMaterial = fast_cast<PhysicalMaterial>(_ci.material);

	bool doDealDamage = true;
	bool dealtDamage = false;

	Vector3 collisionLocation;
	Framework::Room * collidedInRoom;
	if (_ci.collisionLocation.is_set())
	{
		collisionLocation = _ci.collisionLocation.get();
		collidedInRoom = cast_to_nonconst(_ci.collidedInRoom.get());
	}
	else
	{
		collisionLocation = get_owner()->get_presence()->get_placement().get_translation();
		collidedInRoom = get_owner()->get_presence()->get_in_room();
	}

	bool canRicochetNow = false;
	bool canPenetrateNow = false;
	bool ricochetedDueToMaterial = false;
	if (auto* m = fast_cast<Framework::ModuleMovementProjectile>(get_owner()->get_movement()))
	{
		canRicochetNow = m->can_ricochet();
		canPenetrateNow = m->can_penetrate();
	}

	bool hitHealth = false;

	//if (!hitImo && _ci.presenceLink)
	//{
	//	// this is for temporary objects that are not actual collidable objects but in some cases we desire to treat them so.
	//	hitImo = _ci.presenceLink->get_modules_owner();
	//}
	if (hitImo && doDealDamage)
	{
#ifdef INSPECT_PROJECTILE_DAMAGE
		output(TXT("[projectile] to'%p do damage"), get_owner());
#endif
		Damage dealDamage = damage;
		ContinuousDamage dealContinuousDamage = continuousDamage;
		ApplyDamageContext adc;
		adc.randSeed = (int)((pointerint)hitImo + (pointerint)get_owner());
		adc.can_ricochet(canRicochetNow);
#ifdef INSPECT_PROJECTILE_DAMAGE
		output(TXT("[projectile] to'%p pre apply \"useForProjectile\" damage %.2f"), get_owner(), dealDamage.damage.as_float());
#endif
		dealDamage.apply(useForProjectile, &adc);
#ifdef INSPECT_PROJECTILE_DAMAGE
		output(TXT("[projectile] to'%p post apply \"useForProjectile\" damage %.2f"), get_owner(), dealDamage.damage.as_float());
#endif
		if (dealContinuousDamage.is_valid())
		{
			dealContinuousDamage.apply(useForProjectile, &adc); // use adc as we may want to ricochet 100% drain damage
		}
		ricochetedDueToMaterial = adc.out_ricocheted;
		canRicochetNow = adc.out_ricocheted;

		if (hitImo &&
			hitImo->get_top_instigator() == damageInfo.instigator.get())
		{
			doDealDamage = false;

			auto * hi = hitImo;
			while (hi)
			{
				if (auto* h = hi->get_custom<CustomModules::Health>())
				{
					if (h->does_allow_hit_from_same_instigator())
					{
						doDealDamage = true;
					}
				}
				auto* prevHi = hi;
				hi = hi->get_presence()? hi->get_presence()->get_attached_to() : nullptr;
				if (hi == prevHi || dealDamage.damageOnlyToHitObject)
				{
					break;
				}
			}
			if (! doDealDamage)
			{
#ifdef INSPECT_PROJECTILE_LIFETIME
				output(TXT("[projectile] to'%p no damage"), get_owner());
#endif
				dealDamage.damage = Energy::zero(); // no self damage when shooting projectiles
				dealContinuousDamage.damage = Energy::zero();
				dealContinuousDamage.time = 0.0f;
				dealContinuousDamage.minTime = 0.0f;
			}
		}

		if (doDealDamage)
		{
#ifdef INSPECT_PROJECTILE_LIFETIME
			output(TXT("[projectile] to'%p do damage %.2f"), get_owner(), dealDamage.damage.as_float());
#endif
			{
				auto* hi = hitImo;
				while (hi)
				{
					if (auto* mp = hi->get_gameplay_as<ModulePilgrim>())
					{
						if (auto* mexm = mp->get_equipped_exm(EXMID::Passive::projectile_energy_absorber()))
						{
							float useEnergyPt = mexm->get_owner()->get_variables().get_value<float>(NAME(projectileEnergyAbsorberCoef), 1.0f);

							Energy energyToAccumulateDamage = dealDamage.damage.mul(useEnergyPt);
							Energy energyToAccumulateDamageC = dealContinuousDamage.damage.mul(useEnergyPt);

							Energy energyToAccumulate = energyToAccumulateDamage + energyToAccumulateDamageC;
							Energy energyAccumulated = Energy::zero();

							for_count(int, iHand, Hand::MAX)
							{
								Hand::Type hand = (Hand::Type)iHand;
								if (auto* h = mp->get_hand(hand))
								{
									if (auto* iet = fast_cast<IEnergyTransfer>(h->get_gameplay()))
									{
										EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
										etr.energyRequested = energyToAccumulate;
										iet->handle_ammo_energy_transfer_request(etr);
										energyAccumulated += etr.energyResult;
									}
								}
							}

							if (!energyAccumulated.is_zero())
							{
								if (auto* s = mexm->get_owner()->get_sound())
								{
									s->play_sound(NAME(absorbed));
								}
							}
						}
					}
					auto* prevHi = hi;
					hi = hi->get_presence() ? hi->get_presence()->get_attached_to() : nullptr;
					if (hi == prevHi)
					{
						break;
					}
				}
			}

			auto * hi = hitImo;
			while (hi)
			{
				if (auto* h = hi->get_custom<CustomModules::Health>())
				{
					hitHealth = true;
					if (explodeOnHitHealth && !isPenetrator)
					{
						explode(nullptr, NP, hitImo);
						return true;
					}

					if (dealDamageOnHit)
					{
						if (_ci.presenceLink)
						{
							if (_ci.collisionLocation.is_set())
							{
								damageInfo.hitRelativeLocation = _ci.presenceLink->get_placement_in_room().location_to_local(_ci.collisionLocation.get());
							}
							if (_ci.collisionNormal.is_set())
							{
								damageInfo.hitRelativeNormal = _ci.presenceLink->get_placement_in_room().vector_to_local(_ci.collisionNormal.get());
							}
							auto* pl = hi->get_presence()->get_presence_links();
							while (pl)
							{
								if (pl->get_in_room() == get_owner()->get_presence()->get_in_room())
								{
									damageInfo.hitRelativeDir = pl->get_placement_in_room().vector_to_local(get_owner()->get_presence()->get_prev_velocity_linear().normal());
									break;
								}
								pl = pl->get_next_in_object();
							}
						}
						else if (collidedInRoom)
						{
							auto* pl = hi->get_presence()->get_presence_links();
							while (pl)
							{
								if (pl->get_in_room() == collidedInRoom)
								{
									damageInfo.hitRelativeLocation = pl->get_placement_in_room().location_to_local(collisionLocation);
									damageInfo.hitRelativeDir = pl->get_placement_in_room().vector_to_local(get_owner()->get_presence()->get_prev_velocity_linear().normal());
									break;
								}
								pl = pl->get_next_in_object();
							}
						}
#ifdef INSPECT_PROJECTILE_LIFETIME
						output(TXT("[projectile] to'%p deal damage %.2f to \"%S\""), get_owner(), dealDamage.damage.as_float(), h->get_owner()->ai_get_name().to_char());
#endif
						// do even 0 damage to check if we should spawn particles
						// adjust_damage_on_hit_with_extra_effects called above
						h->deal_damage(dealDamage, damage, damageInfo);
						if (dealContinuousDamage.is_valid())
						{
							h->add_continuous_damage(dealContinuousDamage, damageInfo);
						}
						dealtDamage = true;
					}
					break;
				}
				auto* prevHi = hi;
				hi = hi->get_presence() ? hi->get_presence()->get_attached_to() : nullptr;
				if (hi == prevHi || dealDamage.damageOnlyToHitObject)
				{
					break;
				}
			}
		}
	}
	else
	{
#ifdef INSPECT_PROJECTILE_DAMAGE
		output(TXT("[projectile] to'%p don't do damage"), get_owner());
#endif
		// not dealing damage
		Damage dealDamage = damage; // won't be used, we need a copy and this is just to get ricocheting right
		ApplyDamageContext adc;
		adc.randSeed = (int)((pointerint)hitImo + (pointerint)get_owner());
		adc.can_ricochet(canRicochetNow);
		dealDamage.apply(useForProjectile, &adc);
		ricochetedDueToMaterial = adc.out_ricocheted;
		canRicochetNow = adc.out_ricocheted;
	}

	if (auto * r = collidedInRoom)
	{
		if (auto * w = r->get_in_world())
		{
			if (Framework::AI::Message* message = w->create_ai_message(NAME(projectileHit)))
			{
				message->to_room(r);
				message->access_param(NAME(instigator)).access_as<SafePtr<Framework::IModulesOwner>>() = get_owner()->get_valid_top_instigator();
				message->access_param(NAME(hit)).access_as<SafePtr<Framework::IModulesOwner>>() = hitImo && hitImo->is_available_as_safe_object()? cast_to_nonconst(hitImo) : nullptr;
				message->access_param(NAME(inRoom)).access_as<SafePtr<Framework::Room>>() = r;
				message->access_param(NAME(damage)).access_as<Damage>() = damage;

				if (_ci.collisionLocation.is_set())
				{
					message->access_param(NAME(inRoomLocation)).access_as<Vector3>() = _ci.collisionLocation.get();
				}
				if (_ci.collisionNormal.is_set())
				{
					message->access_param(NAME(inRoomNormal)).access_as<Vector3>() = _ci.collisionNormal.get();
				}
			}
		}
	}

#ifndef NO_TEMPORARY_OBJECTS_ON_PROJECTILE_HIT
	if (damageInfo.spawnParticles)
	{
		if (auto * imoP = get_owner())
		{
			bool spawnOnHit = true;
			auto* collidedWithIMO = fast_cast<Framework::IModulesOwner>(cast_to_nonconst(_ci.collidedWithObject.get()));
			if (collidedWithIMO)
			{
				if (auto* ai = collidedWithIMO->get_ai())
				{
					if (ai->is_controlled_by_player())
					{
						todo_note(TXT("for time being we don't spawn anything via material when player is hit"));
						spawnOnHit = false;
					}
				}
			}
			bool spawnOnRicochet = canRicochetNow;
			if (moduleProjectileData)
			{
				auto const & fpm = moduleProjectileData->get_for_physical_material(useForProjectile ? useForProjectile->provideForProjectile : moduleProjectileData->defaultForPhysicalMaterial.providedForProjectile);
				auto const& spawnOnHit = (spawnOnRicochet && (!fpm.spawnOnRicochet.is_empty() || fpm.noSpawnOnRicochet))? fpm.spawnOnRicochet : fpm.spawnOnHit;
				for_every(spawn, spawnOnHit)
				{
					if (auto * particleType = spawn->get())
					{
						if (auto * subWorld = imoP->get_as_temporary_object()->get_in_sub_world())
						{
							if (auto* particles = subWorld->get_one_for(particleType, collidedInRoom, collisionLocation))
							{
#ifdef LOG_PROJECTILE_SPAWN_ON_HIT
								output(TXT("[projectile on hit] spawn \"%S\" (projectile's spawnOnHit)"), particleType->get_name().to_string().to_char());
#endif
								particles->set_creator(get_owner());
								particles->set_instigator(collidedWithIMO);
								set_emissive_to(particles);
								if (_ci.collidedWithShape &&
									_ci.collidedWithShape->get_collidable_shape_bone().is_valid() &&
									_ci.presenceLink &&
									collidedWithIMO && collidedWithIMO->get_appearance()->get_skeleton())
								{
									Name bone = _ci.collidedWithShape->get_collidable_shape_bone();
									int boneIndex = _ci.collidedWithShape->get_collidable_shape_bone_index(collidedWithIMO->get_appearance()->get_skeleton()->get_skeleton());
									an_assert(collidedWithIMO->get_appearance()->get_skeleton() && collidedWithIMO->get_appearance()->get_skeleton()->get_skeleton()->find_bone_index(bone) == boneIndex);
									Transform bonePlacementMS = collidedWithIMO->get_appearance()->get_final_pose_MS()->get_bone(boneIndex);
									Transform bonePlacementWS = collidedWithIMO->get_appearance()->get_ms_to_ws_transform().to_world(bonePlacementMS);
									Transform placementWS = Transform(collisionLocation, _ci.collisionNormal.is_set() && !_ci.collisionNormal.get().is_almost_zero() ? up_matrix33(_ci.collisionNormal.get().normal()).to_quat() : Quat::identity);
									if (collidedWithIMO->get_presence()->get_in_room() != collidedInRoom)
									{
										Framework::RelativeToPresencePlacement rpp;
										rpp.be_temporary_snapshot();
										if (rpp.find_path(collidedWithIMO, collidedInRoom, placementWS))
										{
											placementWS = rpp.from_target_to_owner(placementWS);
										}
									}
									particles->on_activate_attach_to_bone(collidedWithIMO, bone, false, bonePlacementWS.to_local(placementWS));
								}
								else
								{
									particles->on_activate_place_in(collidedInRoom, Transform(collisionLocation, _ci.collisionNormal.is_set() && !_ci.collisionNormal.get().is_almost_zero() ? up_matrix33(_ci.collisionNormal.get().normal()).to_quat() : Quat::identity));
								}
								particles->mark_to_activate();
							}
						}
					}
				}
			}

			if (spawnOnHit)
			{
				if (spawnPhysicalMaterialProjectiles && useForProjectile)
				{
					auto const& spawnOnHit = (spawnOnRicochet && (!useForProjectile->spawnOnRicochet.is_empty() || useForProjectile->noSpawnOnRicochet)) ? useForProjectile->spawnOnRicochet : useForProjectile->spawnOnHit;
					for_every(spawn, spawnOnHit)
					{
						if (auto * particleType = spawn->get())
						{
							if (auto * subWorld = imoP->get_as_temporary_object()->get_in_sub_world())
							{
								if (auto* particles = subWorld->get_one_for(particleType, collidedInRoom, collisionLocation))
								{
#ifdef LOG_PROJECTILE_SPAWN_ON_HIT
									output(TXT("[projectile on hit] spawn \"%S\" (physical material's useForProjectile)"), particleType->get_name().to_string().to_char());
#endif
									auto* collidedWithIMO = fast_cast<Framework::IModulesOwner>(cast_to_nonconst(_ci.collidedWithObject.get()));
									particles->set_creator(get_owner());
									particles->set_instigator(collidedWithIMO);
									// don't set emissive to particles from material - they should be left as they are
									if (_ci.collidedWithShape &&
										_ci.collidedWithShape->get_collidable_shape_bone().is_valid() &&
										_ci.presenceLink &&
										collidedWithIMO->get_appearance()->get_skeleton())
									{
										Name bone = _ci.collidedWithShape->get_collidable_shape_bone();
										int boneIndex = _ci.collidedWithShape->get_collidable_shape_bone_index(collidedWithIMO->get_appearance()->get_skeleton()->get_skeleton());
										an_assert(collidedWithIMO->get_appearance()->get_skeleton() && collidedWithIMO->get_appearance()->get_skeleton()->get_skeleton()->find_bone_index(bone) == boneIndex);
										Transform bonePlacementMS = collidedWithIMO->get_appearance()->get_final_pose_MS()->get_bone(boneIndex);
										Transform bonePlacementWS = collidedWithIMO->get_appearance()->get_ms_to_ws_transform().to_world(bonePlacementMS);
										Transform placementWS = Transform(collisionLocation, _ci.collisionNormal.is_set() && !_ci.collisionNormal.get().is_almost_zero() ? up_matrix33(_ci.collisionNormal.get().normal()).to_quat() : Quat::identity);
										if (collidedWithIMO->get_presence()->get_in_room() != collidedInRoom)
										{
											Framework::RelativeToPresencePlacement rpp;
											rpp.be_temporary_snapshot();
											if (rpp.find_path(collidedWithIMO, collidedInRoom, placementWS))
											{
												placementWS = rpp.from_target_to_owner(placementWS);
											}
										}
										particles->on_activate_attach_to_bone(collidedWithIMO, bone, false, bonePlacementWS.to_local(placementWS));
									}
									else
									{
										particles->on_activate_place_in(collidedInRoom, Transform(collisionLocation, _ci.collisionNormal.is_set() && !_ci.collisionNormal.get().is_almost_zero() ? up_matrix33(_ci.collisionNormal.get().normal()).to_quat() : Quat::identity));
									}
									particles->mark_to_activate();
								}
							}
						}
					}
				}
			}
		}
	}
#endif

	bool shouldCease = ceaseOnHit || (ceaseOnDealDamage && dealtDamage);

	/*
	if (useForProjectile && !useForProjectile->ricochetAmount.is_zero() && canRicochetNow)
	{
		shouldCease = false;
#ifdef DRAW_INSPECT_PROJECTILE_LIFETIME
		debug_context(get_owner()->get_presence()->get_in_room());
		debug_draw_time_based(2.0f, debug_draw_text(true, Colour::cyan, get_owner()->get_presence()->get_placement().get_translation() + Vector3(0.0f, 0.0f, 0.01f), Vector2::half, true, 0.3f, false, TXT("ri")));
		debug_no_context();
#endif
	}
	*/

	if (isPenetrator && hitHealth && canPenetrateNow)
	{
		shouldCease = false;
		if (hitImo)
		{
			if (auto* c = get_owner()->get_collision())
			{
				c->dont_collide_with_up_to_top_instigator(hitImo, 0.1f);
			}
		}
#ifdef DRAW_INSPECT_PROJECTILE_LIFETIME
		debug_context(get_owner()->get_presence()->get_in_room());
		debug_draw_time_based(2.0f, debug_draw_text(true, Colour::magenta, get_owner()->get_presence()->get_placement().get_translation(), Vector2::half, true, 0.35f, false, TXT("P")));
		debug_no_context();
#endif
	}

	if (!dealtDamage || ricochetedDueToMaterial)
	{
		if (auto* m = fast_cast<Framework::ModuleMovementProjectile>(get_owner()->get_movement()))
		{
			if (m->can_ricochet())
			{
				if (_ci.collisionNormal.is_set())
				{
					Vector3 velocityLinear = get_owner()->get_presence()->get_velocity_linear();
					Vector3 hitNormal = _ci.collisionNormal.get();
					float dot = Vector3::dot(hitNormal, velocityLinear);
					if (dot < 0.0f)
					{
						velocityLinear = velocityLinear - hitNormal * (dot * 2.0f);
						get_owner()->get_presence()->set_velocity_linear(velocityLinear);
					}
				}

				if (ricochetedDueToMaterial)
				{
					// stay alive
					shouldCease = false;
				}
				else
				{
					++ricocheted;
					if (ricochetLimit < 0 || ricocheted <= ricochetLimit)
					{
						// stay alive
						shouldCease = false;
					}
				}

				if (!shouldCease)
				{
#ifdef INSPECT_PROJECTILE_LIFETIME
					output(TXT("[projectile] to'%p ricochets [R]"), get_owner());
#endif
#ifdef DRAW_INSPECT_PROJECTILE_LIFETIME
					debug_context(get_owner()->get_presence()->get_in_room());
					debug_draw_time_based(2.0f, debug_draw_text(true, Colour::red, get_owner()->get_presence()->get_placement().get_translation(), Vector2::half, true, 0.3f, false, TXT("R")));
					debug_no_context();
#endif
				}
			}
		}
	}

	if (! shouldCease)
	{
		ApplyDamageContext adc;
		adc.randSeed = (int)((pointerint)hitImo + (pointerint)get_owner());
		adc.can_ricochet(canRicochetNow);
		Energy preAdjustmentDamage = damage.damage;
		Energy preAdjustmentContinuousDamage = continuousDamage.damage;
		damage.apply_post_impact(useForProjectile, &adc);
		continuousDamage.apply_post_impact(useForProjectile, &adc);
		if (damage.damage < maxDamageAmount.mul(magic_number 0.05f) &&
			preAdjustmentDamage < maxDamageAmount.mul(magic_number 0.5f) && // the second check is to always allow ricocheting at least once
			continuousDamage.damage < maxDamageAmount.mul(magic_number 0.05f) &&
			preAdjustmentContinuousDamage < maxDamageAmount.mul(magic_number 0.5f)) // the second check is to always allow ricocheting at least once
		{
			// way to low, just cease it
			shouldCease = true;
		}
		else
		{
			// we should be able to collide with owner as we ricocheted from something
			if (auto* c = get_owner()->get_collision())
			{
				c->clear_dont_collide_with();
			}
		}
	}

	if (shouldCease)
	{
		if (explodeOnHit && isPenetrator)
		{
			explode(nullptr, NP, hitImo);
			return true;
		}
		else
		{
#ifdef INSPECT_PROJECTILE_LIFETIME
			output(TXT("[projectile] to'%p cease to exist during process hit"), get_owner());
#endif
			Framework::ParticlesUtils::desire_to_deactivate(get_owner());
			get_owner()->cease_to_exist(true); // it's okay here as processing hit may happen only post move
		}
	}
	else
	{
#ifdef INSPECT_PROJECTILE_LIFETIME
		output(TXT("[projectile] to'%p keeps existing [I]"), get_owner());
#endif
#ifdef DRAW_INSPECT_PROJECTILE_LIFETIME
		debug_context(get_owner()->get_presence()->get_in_room());
		debug_draw_time_based(2.0f, debug_draw_text(true, Colour::cyan, get_owner()->get_presence()->get_placement().get_translation() + Vector3(0.0f, 0.0f, 0.01f), Vector2::half, true, 0.3f, false, TXT("!")));
		debug_no_context();
#endif
	}

	return false;
}

void ModuleProjectile::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	travelledDistance += get_owner()->get_presence()->get_prev_placement_offset().get_translation().length();
	if (travelledDistanceActive > 0.0f)
	{
		travelledDistanceActive = max(0.0f, travelledDistanceActive - _deltaTime);
		if (auto* ap = get_owner()->get_appearance())
		{
			ap->set_shader_param(NAME(projectileTravelledDistance), travelledDistance);
			ap->set_shader_param(NAME(projectileTravelledDistanceActive), travelledDistanceActive);
		}
	}

	if (speedToCeaseExist > 0.0f && get_owner()->get_presence()->get_velocity_linear().length_squared() <= sqr(speedToCeaseExist))
	{
		timeToLive = 0.0f;
	}

	if (timeToLive <= 0.0f)
	{
		if (auto* h = get_owner()->get_custom<CustomModules::Health>())
		{
			// because we are temporary object and we may depend on whether our existence is particle based, make sure we want to stop any continuous damage to detach things
			h->clear_continuous_damage();
		}
	}

	if (!explodeOnCollisionWithFlags.is_empty())
	{
		if (auto* mc = get_owner()->get_collision())
		{
			Framework::IModulesOwner* collisionDetectedWith = nullptr;
			for_every(collidedWith, mc->get_collided_with())
			{
				if (auto* imo = fast_cast<Framework::IModulesOwner>(collidedWith->collidedWithObject.get()))
				{
					if (auto* imomc = imo->get_collision())
					{
						if (Collision::Flags::check(imomc->get_collision_flags(), explodeOnCollisionWithFlags))
						{
							collisionDetectedWith = imo;
							break;
						}
					}
				}
			}
			if (collisionDetectedWith)
			{
				explode(nullptr, NP, collisionDetectedWith);
			}
		}
	}
}

bool ModuleProjectile::on_death(Damage const& _damage, DamageInfo const& _damageInfo)
{
	if (!_damageInfo.peacefulDamage && explodeOnDeath)
	{
		explode(_damageInfo.instigator.get(), _damage.damageType);
	}
	return false;
}

void ModuleProjectile::explode(Framework::IModulesOwner* _instigator, Optional<DamageType::Type> const& _damageType, Framework::IModulesOwner* _forceFullDamageOnTo)
{
	if (exploded)
	{
		return;
	}
	exploded = true;

#ifdef INSPECT_PROJECTILE_LIFETIME
	output(TXT("[projectile] to'%p explode!"), get_owner());
#endif

	if (! damage.damage.is_zero() ||
		continuousDamage.is_valid())
	{
		if (auto* tos = get_owner()->get_temporary_objects())
		{
			if (!_instigator)
			{
				_instigator = get_owner()->get_valid_top_instigator();
			}
			Array<SafePtr<Framework::IModulesOwner>> spawned;
			Name tryExplosion = NAME(explode);
			if (_damageType.is_set())
			{
				switch (_damageType.get())
				{
				case DamageType::Plasma: tryExplosion = NAME(explodePlasma); break;
				case DamageType::Ion: tryExplosion = NAME(explodeIon); break;
				case DamageType::Corrosion: tryExplosion = NAME(explodeCorrosion); break;
				case DamageType::Lightning: tryExplosion = NAME(explodeLightning); break;
				default:
					todo_implement;
				}
			}
			tos->spawn_all(tryExplosion, NP, &spawned);
			if (spawned.is_empty() &&
				tryExplosion != NAME(explode))
			{
				tos->spawn_all(NAME(explode), NP, &spawned);
			}
			if (spawned.is_empty())
			{
				warn(TXT("\"%S\" doesn't have a way to explode"), get_owner()->ai_get_name().to_char());
			}
			for_every_ref(s, spawned)
			{
				s->set_creator(get_owner());
				s->set_instigator(_instigator);

				if (auto* to = s->get_as_temporary_object())
				{
					{
						// calculate how much damage from this projectile goes to explosion
						EnergyCoef _useD = explosionDamageCoef;
						EnergyCoef _useCD = explosionContinuousDamageCoef;

						Energy _useDamage = damage.damage;
						Energy _useContinuousDamage = continuousDamage.damage;

						bool _useRedistributeEnergyOnExplosion = redistributeEnergyOnExplosion;
						to->perform_on_activate([_useD, _useCD, _useDamage, _useContinuousDamage, _forceFullDamageOnTo, _useRedistributeEnergyOnExplosion](Framework::TemporaryObject* to)
						{
							if (auto* e = to->get_gameplay_as<ModuleExplosion>())
							{
								// maybe the explosion has some parameter that overrides our general value?
								EnergyCoef useD = to->get_variables().get_value<EnergyCoef>(NAME(explosionDamageCoef), _useD);
								EnergyCoef useCD = to->get_variables().get_value<EnergyCoef>(NAME(explosionContinuousDamageCoef), _useCD);
								bool useRedistributeEnergyOnExplosion = to->get_variables().get_value<bool>(NAME(redistributeEnergyOnExplosion), _useRedistributeEnergyOnExplosion);

								if (useD.is_negative())
								{
									useD = useCD;
								}
								if (useCD.is_negative())
								{
									useCD = useD;
								}
								if (!useD.is_negative() &&
									!useCD.is_negative())
								{
									// calculate how much damage from this projectile goes to explosion
									an_assert(!useD.is_negative());
									an_assert(!useCD.is_negative());

#ifdef INSPECT_DAMAGE
									output(TXT("explode using damage: %.2fx%.2f%% + %.2fx%.2f%%"),
										_useDamage.as_float(), useD.as_float() * 100.0f,
										_useContinuousDamage.as_float(), useCD.as_float() * 100.0f);
#endif

									Energy useDamage = _useDamage.adjusted(useD);
									Energy useContinuousDamage = _useContinuousDamage.adjusted(useCD);

									if (useRedistributeEnergyOnExplosion)
									{
										// now let's redistribute our total damage according to how it is distributed in explosion we use - maintain proportion
										Energy orgDamage = e->access_damage().damage;
										Energy orgContinuousDamage = e->access_continuous_damage().is_valid() ? e->access_continuous_damage().damage : Energy::zero();

										// keep continuous damage as is, we have weapon modifiers working hard on keeping those
#ifdef INSPECT_DAMAGE
										output(TXT("explode redistribute damage: %.2f + %.2f"), useDamage.as_float(), useContinuousDamage.as_float());
										output(TXT("to fit: %.2f + %.2f"), orgDamage.as_float(), orgContinuousDamage.as_float());
#endif
										if (orgContinuousDamage.is_zero())
										{
											useDamage = useDamage + useContinuousDamage;
											useContinuousDamage = Energy::zero();
										}
										else if (orgDamage.is_zero())
										{
											useContinuousDamage = useDamage + useContinuousDamage;
											useDamage = Energy::zero();
										}
										else
										{
											float proportion = orgDamage.div_to_float(orgDamage + orgContinuousDamage);
											Energy newDamageSum = useDamage + useContinuousDamage;
											useDamage = newDamageSum.mul(proportion);
											useContinuousDamage = newDamageSum - useDamage;
#ifdef INSPECT_DAMAGE
											output(TXT("using proportion %.2f%%"), proportion * 100.0f);
#endif
										}
									}

#ifdef INSPECT_DAMAGE
									output(TXT("explode damage (redistributed): %.2f + %.2f"), useDamage.as_float(), useContinuousDamage.as_float());
#endif

									// and store new values
									e->access_damage().damage = useDamage;
									e->access_continuous_damage().damage = useContinuousDamage;
									if (_forceFullDamageOnTo)
									{
										e->force_full_damage_for(_forceFullDamageOnTo);
									}
								}
							}
						});
					}
					{
						Range _explosionRange = explosionRange;
						to->perform_on_activate([_explosionRange](Framework::TemporaryObject* to)
						{
							Range explosionRange = to->get_variables().get_value<Range>(NAME(explosionRange), _explosionRange);
							if (!explosionRange.is_zero())
							{
								if (auto* e = to->get_gameplay_as<ModuleExplosion>())
								{
									e->access_range() = explosionRange;
								}
							}
						});
					}
				}
				else
				{
					todo_implement;
				}
			}
		}
		damage.damage = Energy::zero();
		continuousDamage.damage = Energy::zero();
	}
	// evaporate quickly (will still appear for a moment)
	timeToLive = 0.0f;
	scaleBlendTime = 0.05f;
}

Array<Framework::LocalisedString> const* ModuleProjectile::get_additional_gun_stats_info() const
{
	if (moduleProjectileData)
	{
		return &moduleProjectileData->additionalGunStatsInfo;
	}
	return nullptr;
}

//

REGISTER_FOR_FAST_CAST(ModuleProjectileData);

ModuleProjectileData::ModuleProjectileData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleProjectileData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	forPhysicalMaterials.clear();

	bool result = base::load_from_xml(_node, _lc);
	
	for_every(node, _node->children_named(TXT("additionalGunStatsInfo")))
	{
		Framework::LocalisedString ls;
		if (ls.load_from_xml(node, _lc, String::printf(TXT("additionalGunStatsInfo_%i"), additionalGunStatsInfo.get_size()).to_char()))
		{
			additionalGunStatsInfo.push_back(ls);
		}
		else
		{
			result = false;
		}
	}
	result &= projectileType.load_from_xml_attribute_or_child_node(_node, TXT("projectileType"));
	
	return result;
}

bool ModuleProjectileData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("projectileType") ||
		_node->get_name() == TXT("additionalGunStatsInfo"))
	{
		// already handed in load_from_xml
		return true;
	}
	if (_node->get_name() == TXT("forPhysicalMaterial"))
	{
		ForPhysicalMaterial fpm;
		if (fpm.load_from_xml(_node, _lc))
		{
			if (fpm.providedForProjectile.is_empty())
			{
				defaultForPhysicalMaterial = fpm;
			}
			else
			{
				forPhysicalMaterials.push_back(fpm);
			}
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("problem loading forPhysicalMaterial"));
			return false;
		}
	}
	return base::read_parameter_from(_node, _lc);
}

bool ModuleProjectileData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= defaultForPhysicalMaterial.prepare_for_game(_library, _pfgContext);
	for_every(fpm, forPhysicalMaterials)
	{
		result &= fpm->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

ModuleProjectileData::ForPhysicalMaterial const & ModuleProjectileData::get_for_physical_material(Tags const & _providedForProjectile) const
{
	if (!_providedForProjectile.is_empty())
	{
		for_every(fpm, forPhysicalMaterials)
		{
			if (fpm->providedForProjectile.does_match_any_from(_providedForProjectile))
			{
				return *fpm;
			}
		}
	}
	return defaultForPhysicalMaterial;
}

//

bool ModuleProjectileData::ForPhysicalMaterial::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= providedForProjectile.load_from_xml_attribute_or_child_node(_node, TXT("provided"));

	for_every(node, _node->children_named(TXT("spawnOnHit")))
	{
		Framework::UsedLibraryStored<Framework::TemporaryObjectType> temporaryObjectType;
		if (temporaryObjectType.load_from_xml(node, TXT("temporaryObjectType"), _lc) &&
			temporaryObjectType.is_name_valid())
		{
			spawnOnHit.push_back(temporaryObjectType);
		}
		else
		{
			error_loading_xml(node, TXT("temporaryObjectType not provided"));
			result = false;
		}
	}

	noSpawnOnRicochet = _node->get_bool_attribute_or_from_child_presence(TXT("noSpawnOnRicochet"), noSpawnOnRicochet);

	for_every(node, _node->children_named(TXT("spawnOnRicochet")))
	{
		Framework::UsedLibraryStored<Framework::TemporaryObjectType> temporaryObjectType;
		if (temporaryObjectType.load_from_xml(node, TXT("temporaryObjectType"), _lc) &&
			temporaryObjectType.is_name_valid())
		{
			spawnOnRicochet.push_back(temporaryObjectType);
		}
		else
		{
			error_loading_xml(node, TXT("temporaryObjectType not provided"));
			result = false;
		}
	}

	return result;
}

bool ModuleProjectileData::ForPhysicalMaterial::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(spawn, spawnOnHit)
	{
		result &= spawn->prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	}

	for_every(spawn, spawnOnRicochet)
	{
		result &= spawn->prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}
