#pragma once

#include "..\game\energy.h"

#include "..\..\core\tags\tag.h"
#include "..\..\framework\collision\physicalMaterial.h"

namespace Framework
{
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	struct Damage;

	class PhysicalMaterial
	: public Framework::PhysicalMaterial
	{
		FAST_CAST_DECLARE(PhysicalMaterial);
		FAST_CAST_BASE(Framework::PhysicalMaterial);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::PhysicalMaterial base;
	public:
		PhysicalMaterial(Framework::Library * _library, Framework::LibraryName const & _name);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~PhysicalMaterial();

	public:
		struct ForProjectile
		{
			// conditions
			TagCondition forProjectileType; // to catch right kind of projectile
			EnergyCoefRange forArmourPiercing = EnergyCoefRange::empty;

			// adjustments and effects
			Tags provideForProjectile; // this is used so projectile may react to physical material properly
			EnergyCoef adjustDamage = EnergyCoef(1); // this always modifies damage, may make objects indestructible
			EnergyCoef adjustArmourDamage = EnergyCoef(1); // this is used for armour - armour piercing at 100% will ignore this coef - allows armour to be pierced
			// how ricocheting works
			//	forceRicochet means that projectile's armour piercing does nothing
			//	ricochetAmount tells how much of projectile is ricocheted (depends on armour piercing! with 100% armour piercing, unless ricocheting is forced projectile is not ricocheted at all)
			//	ricochetDamage tells how much of what was left after ricocheting (ricochetAmount) stays in the procjetile
			EnergyCoef ricochetAmount = EnergyCoef(0); // how much does it ricochet (some energy stays, some ricochets)
			bool forceRicochet = false; // if set to true, will ignore armour piercing component
			Optional<float> ricochetChance;
			Optional<float> ricochetChanceDamageBased; // will be used with current damage to calculate used chance, multiplied by ricochetChance
			EnergyCoef ricochetDamage = EnergyCoef(1); // how much damage is left after ricochet
			Array<Framework::UsedLibraryStored<Framework::TemporaryObjectType>> spawnOnHit;
			Array<Framework::UsedLibraryStored<Framework::TemporaryObjectType>> spawnOnRicochet;
			bool noSpawnOnRicochet = false; // has to be explicitly provided

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		};

		ForProjectile const & get_for_projectile(Damage const& _damage, Tags const & _projectile = Tags::none) const;

		static ForProjectile const * get_for_projectile(Collision::PhysicalMaterial const * _material, Damage const& _damage, Tags const & _projectile = Tags::none);

	protected:
		ForProjectile defaultForProjectile;
		Array<ForProjectile> forProjectiles; // they have to be in priority, the first should be the most specific, the last, the least
	};
};
