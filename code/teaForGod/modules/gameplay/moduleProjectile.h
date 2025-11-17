#pragma once

#include "..\..\game\damage.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\module\moduleGameplayProjectile.h"
#include "..\..\..\framework\text\localisedString.h"

namespace Framework
{
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;
	class ModuleProjectileData;

	class ModuleProjectile
	: public Framework::ModuleGameplayProjectile
	{
		FAST_CAST_DECLARE(ModuleProjectile);
		FAST_CAST_BASE(Framework::ModuleGameplayProjectile);
		FAST_CAST_END();

		typedef Framework::ModuleGameplayProjectile base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleProjectile(Framework::IModulesOwner* _owner);
		virtual ~ModuleProjectile();

		Damage & access_damage() { return damage; }
		ContinuousDamage & access_continuous_damage() { return continuousDamage; }
		void set_anti_deflection(float _antiDeflection) { antiDeflection = _antiDeflection; }

		bool is_deflectable() const { return isDeflectable && antiDeflection != 1.0f; }
		float get_anti_deflection() const { return isDeflectable ? antiDeflection : 1.0f; }

		void be_penetrator(bool _penetrator = true) { isPenetrator = _penetrator; }

		void explode(Framework::IModulesOwner* _instigator = nullptr, Optional<DamageType::Type> const & _damageType = NP, Framework::IModulesOwner* _forceFullDamageOnTo = nullptr);

		Array<Framework::LocalisedString> const * get_additional_gun_stats_info() const;

		enum UpdateAppearanceVariables
		{
			UAV_Speed		= 1 << 0,
			UAV_Damage		= 1 << 1,
			UAV_Emissives	= 1 << 2,

			UAV_All = UAV_Speed | UAV_Damage | UAV_Emissives
		};

		void update_appearance_variables(int _flags = UAV_All);

		bool on_death(Damage const& _damage, DamageInfo const& _damageInfo);

		void force_abnormal_end(Framework::IModulesOwner* _instigator = nullptr); // explode/disappear

	public: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const * _moduleData);
		override_ void activate();

	protected: // ModuleGameplayProjectile
		override_ bool process_hit(Framework::CollisionInfo const & _ci);
		override_ void advance_post_move(float _deltaTime);

	private:
		ModuleProjectileData const * moduleProjectileData = nullptr;
		
		// setup - should have all those variables set!
		bool autoGetEmissiveColours = true;
		Damage damage;
		ContinuousDamage continuousDamage;
		Energy maxDamageAmount = Energy::zero(); // which serves as original one, if we drop below a certain value, we cease to exist
		bool isPenetrator = false;
		bool isDeflectable = false;
		float antiDeflection = 0.0f;
		int ricochetLimit = 0; // will work only if a projectile (movement projectile) is marked as being able to ricochet, if the projectile has been ricocheted by the material, this limit is ignored
		bool spawnPhysicalMaterialProjectiles = true;
		float speedToCeaseExist = 0.0f;

		// if ceaseOnHit, any hit will kill it, ceaseOnDealDamage is ignored then
		bool ceaseOnHit = true; // unless ricochets, penetrates
		bool ceaseOnDealDamage = true; // unless ricochets, penetrates
		bool dealDamageOnHit = true;
		bool explodeOnHit = false; // on explosion won't do normal path but instead will just continue with explosion
		bool explodeOnHitHealth = false;
		bool explodeOnDeath = false;
		Collision::Flags explodeOnCollisionWithFlags = Collision::Flags::none();

		// if negative, will keep as it is unless the other is set, then the other is taken for both
		// this is used to make the explosion use same parameters as the projectile (projectile does 10dmg, with coef=200% explosion will do 20dmg)
		// how it actually goes into damage/continuous damage is dependent on the actual explosion setup - we calculate total damage and break into proportion defined by explosion
		// all of these might be overriden by temporary object's variables (that might be set in temporary objects' data)
		EnergyCoef explosionDamageCoef = EnergyCoef(-1.0f);
		EnergyCoef explosionContinuousDamageCoef = EnergyCoef(-1.0f);
		bool redistributeEnergyOnExplosion = false; // will match explosion's redistribution
		Range explosionRange = Range::zero;

		// run
		bool exploded = false;
		int ricocheted = 0;
		Colour projectileEmissiveColour = Colour::none;
		Colour projectileEmissiveBaseColour = Colour::none;
		float travelledDistance = 0.0f;
		float travelledDistanceActive = 1.0f;

		void set_emissive_to(Framework::IModulesOwner* _imo) const;
	};

	class ModuleProjectileData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleProjectileData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();
		typedef Framework::ModuleData base;
	public:
		ModuleProjectileData(Framework::LibraryStored* _inLibraryStored);

		Array<Framework::LocalisedString> const& get_additional_gun_stats_info() const { return additionalGunStatsInfo; }

	protected: // Framework::ModuleData
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		Array<Framework::LocalisedString> additionalGunStatsInfo;

		struct ForPhysicalMaterial
		{
			Tags providedForProjectile; // ForProjectile's provideForProjectile
			Array<Framework::UsedLibraryStored<Framework::TemporaryObjectType>> spawnOnHit;
			Array<Framework::UsedLibraryStored<Framework::TemporaryObjectType>> spawnOnRicochet;
			bool noSpawnOnRicochet = false; // has to be explicitly provided
			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		};
		ForPhysicalMaterial defaultForPhysicalMaterial;
		Array<ForPhysicalMaterial> forPhysicalMaterials;

		Tags projectileType;

		ForPhysicalMaterial const & get_for_physical_material(Tags const & _providedForProjectile) const;

		friend class ModuleProjectile;
	};
};

