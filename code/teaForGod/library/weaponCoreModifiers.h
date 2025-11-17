#pragma once

#include "weaponPartType.h"

#include "..\game\energy.h"

#include "..\..\framework\library\libraryStored.h"

struct Tags;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;
};

namespace TeaForGodEmperor
{
	class WeaponCoreModifiers
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(WeaponCoreModifiers);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		struct ForCore
		{
			// default is 100% == 1.0
			Optional<Colour> overlayInfoColour;
			WeaponPartStatInfo<EnergyCoef> chamberSizeAdj;
			WeaponPartStatInfo<EnergyCoef> damageAdj;
			WeaponPartStatInfo<float> continuousDamageMinTime;
			WeaponPartStatInfo<float> continuousDamageTime;
			WeaponPartStatInfo<EnergyCoef> outputSpeedAdj; // increases both storage's and magazine's
			WeaponPartStatInfo<float> projectileSpeedAdj;
			WeaponPartStatInfo<float> projectileSpreadAdd;
			WeaponPartStatInfo<EnergyCoef> armourPiercing;
			WeaponPartStatInfo<float> antiDeflection;
			WeaponPartStatInfo<float> maxDist; // if used
			Framework::LocalisedString additionalWeaponInfo; // shown in weapon and core
			Framework::LocalisedString additionalCoreInfo; // shown only in core
			bool sightColourSameAsProjectile = false;
			bool noHitBoneDamage = false; // can't do headshot etc
			bool noContinuousFire = false; // can't do continuous fire at all
			bool singleSpecialProjectile = false; // single or special
			Framework::LocalisedString projectileInfo; // instead of count+speed+spread
		};

		ForCore const* get_for_core(WeaponCoreKind::Type _coreKind) const { return forCore.is_index_valid(_coreKind) ? &forCore[_coreKind] : nullptr; }

		void build_overlay_stats_info(WeaponCoreKind::Type _coreKind, List<String>& _list, String const & _linePrefix) const;

	public:
		WeaponCoreModifiers(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~WeaponCoreModifiers();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

	private:
		ArrayStatic<ForCore, WeaponCoreKind::MAX> forCore;
	};
};
