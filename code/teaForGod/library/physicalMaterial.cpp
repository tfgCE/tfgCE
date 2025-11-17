#include "physicalMaterial.h"

#include "..\game\damage.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define INVESTIGATE_USE_FOR_PROJECTILE

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(PhysicalMaterial);
LIBRARY_STORED_DEFINE_TYPE(PhysicalMaterial, physicalMaterial);

PhysicalMaterial::PhysicalMaterial(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
	be<PhysicalMaterial>();
}

PhysicalMaterial::~PhysicalMaterial()
{
}

bool PhysicalMaterial::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	forProjectiles.clear();

	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("forProjectile")))
	{
		ForProjectile fp;
		if (fp.load_from_xml(node, _lc))
		{
			forProjectiles.push_back(fp);
		}
		else
		{
			result = false;
		}
	}

	if (!forProjectiles.is_empty())
	{
		defaultForProjectile = forProjectiles.get_last();
	}

	return result;
}

bool PhysicalMaterial::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	
	result &= defaultForProjectile.prepare_for_game(_library, _pfgContext);
	for_every(fp, forProjectiles)
	{
		result &= fp->prepare_for_game(_library, _pfgContext);
	}
	
	return result;
}

PhysicalMaterial::ForProjectile const & PhysicalMaterial::get_for_projectile(Damage const& _damage, Tags const & _projectile) const
{
#ifdef INVESTIGATE_USE_FOR_PROJECTILE
	output(TXT("find use for projectile \"%S\", armour piercing %S"), _projectile.to_string().to_char(), _damage.armourPiercing.as_string_percentage().to_char());
#endif
	for_every(fp, forProjectiles)
	{
#ifdef INVESTIGATE_USE_FOR_PROJECTILE
		output(TXT(" check %i"), for_everys_index(fp));
#endif
		if (!fp->forArmourPiercing.is_empty() && !fp->forArmourPiercing.does_contain(_damage.armourPiercing))
		{
#ifdef INVESTIGATE_USE_FOR_PROJECTILE
			output(TXT("  armour piercing failed %S"), fp->forArmourPiercing.as_string().to_char());
#endif
			continue;
		}

		if (!fp->forProjectileType.is_empty() && !fp->forProjectileType.check(_projectile))
		{
#ifdef INVESTIGATE_USE_FOR_PROJECTILE
			output(TXT("  for projectile failed %S"), fp->forProjectileType.to_string().to_char());
#endif
			continue;
		}

#ifdef INVESTIGATE_USE_FOR_PROJECTILE
		output(TXT("  good, use it!"));
#endif
		return *fp;
	}

#ifdef INVESTIGATE_USE_FOR_PROJECTILE
	output(TXT(" use default then"));
#endif
	return defaultForProjectile;
}

PhysicalMaterial::ForProjectile const * PhysicalMaterial::get_for_projectile(Collision::PhysicalMaterial const * _material, Damage const& _damage, Tags const & _projectile)
{
	PhysicalMaterial const * useMaterial = fast_cast<PhysicalMaterial>(_material);
	PhysicalMaterial::ForProjectile const * useForProjectile = nullptr;
	if (useMaterial)
	{
		useForProjectile = &useMaterial->get_for_projectile(_damage, _projectile);
	}
	return useForProjectile;
}

//

bool PhysicalMaterial::ForProjectile::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_node->has_attribute(TXT("forProjectileType")))
	{
		error_loading_xml(_node, TXT("use \"type\" instead of \"forProjectileType\""));
		result = false;
	}
	result &= forProjectileType.load_from_xml_attribute_or_child_node(_node, TXT("type"));
	result &= provideForProjectile.load_from_xml_attribute_or_child_node(_node, TXT("provide"));

	forArmourPiercing.load_from_xml(_node, TXT("forArmourPiercing"));
	adjustDamage = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("adjustDamage"), adjustDamage);
	adjustArmourDamage = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("adjustArmourDamage"), adjustArmourDamage);
	forceRicochet = _node->get_bool_attribute_or_from_child_presence(TXT("forceRicochet"), forceRicochet);
	ricochetChance.load_from_xml(_node, TXT("ricochetChance"));
	ricochetChanceDamageBased.load_from_xml(_node, TXT("ricochetChanceDamageBased"));
	ricochetAmount = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("ricochetAmount"), ricochetAmount);
	ricochetDamage = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("ricochetDamage"), ricochetDamage);

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
			error_loading_xml(_node, TXT("temporaryObjectType not provided"));
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
			error_loading_xml(_node, TXT("temporaryObjectType not provided"));
			result = false;
		}
	}

	return result;
}


bool PhysicalMaterial::ForProjectile::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
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
