#pragma once

#include "..\game\damage.h"
#include "..\game\energy.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\text\localisedString.h"

namespace Framework
{
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	struct HealthExtraEffectParams
	{
		Optional<DamageType::Type> ignoreForDamageType; // we may want corrosion to ignore modifying corrosion
		Optional<EnergyCoef> damageCoef;
		Optional<Energy> extraDamageCap; // if damage coef exceeds
		Optional<bool> ricocheting; // block or enforce
		Optional<EnergyCoef> armourPiercing; // goes up to 100% (50% takes from 50% to 75%)
		
		Optional<Colour> globalTintColour;

		// copied to instance
		struct InvestigatorInfo
		{
			Name id; // used for localised strings and by pilgrim overlay info

			// conditions
			Optional<Name> provideDamageCoefAs;
			Optional<Name> provideExtraDamageCapAs;
			Optional<Name> provideArmourPiercingAs; // armour piercing
			Optional<Name> provideArmourPiercingArmourAs; // how armour is affected

			Framework::LocalisedString icon; // this is only used for icon, only first character is used
			Framework::LocalisedString info; // this is only used for long info

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		} investigatorInfo;

		void setup_formatter_params(REF_ Framework::StringFormatterParams& _params) const;
	};

	struct HealthExtraEffect
	: public HealthExtraEffectParams
	{
		Name name;
		Optional<float> time;
		Optional<DamageType::Type> forContinuousDamageType; // this could be in params as it doesn't change but it is better to have it here for clarity
		Name temporaryObject; // from owner's objects
		Framework::UsedLibraryStored<Framework::TemporaryObjectType> temporaryObjectType;

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
	};

	struct DamageRule
	{
		Name id; // more as a reference/info

#ifdef AN_DEVELOPMENT_OR_PROFILER
		String info;
#endif
		int priority = 0;
		int loadingOrder = 0;

		// conditions
		Optional<DamageType::Type> forDamageType;
		Optional<Name> forDamageExtraInfo;
		Optional<bool> forMeleeDamage;
		Optional<bool> forExplosionDamage;
		Optional<Name> ifCurrentExtraEffect; // if there is an extra effect active of this name
		Optional<DamageType::Type> ifCurrentContinuousDamageType; // if there is a currently active continuous damage type
		Optional<bool> forDamagerIsExplosion;
		Optional<bool> forDamagerIsAlly;
		Optional<bool> forDamagerIsSelf;

		// result
		bool kill = false; // will immediately kill
		bool heal = false; // will heal instead of damage (will use cost!)
		bool noDamage = false;
		bool noContinuousDamage = false;
		bool noExplosion = false;
		bool addToProjectileDamage = false; // if it is a projectile - if hit a projectile, will increase the projectile's damage

		Optional<float> spawnTemporaryObjectCooldown;
		Name spawnTemporaryObjectCooldownId;
		bool spawnTemporaryObjectAtDamager = false;
		Name spawnTemporaryObject;
		Framework::UsedLibraryStored<Framework::TemporaryObjectType> spawnTemporaryObjectType;
		Optional<EnergyCoef> temporaryObjectDamageCoef; // from the damage

		Optional<Name> endExtraEffect;
		Optional<DamageType::Type> endContinuousDamageType;

		Optional<EnergyCoef> applyCoef;

		Array<HealthExtraEffect> extraEffects;

		bool stopProcessing = false;

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	public:
		static inline int compare(void const* _a, void const* _b)
		{
			DamageRule const& a = *plain_cast<DamageRule>(_a);
			DamageRule const& b = *plain_cast<DamageRule>(_b);
			if (a.priority > b.priority) return A_BEFORE_B;
			if (a.priority < b.priority) return B_BEFORE_A;
			if (a.loadingOrder < b.loadingOrder) return A_BEFORE_B;
			if (a.loadingOrder > b.loadingOrder) return B_BEFORE_A;
			return A_AS_B;
		}

		static void set_loading_order(Array<DamageRule>& _dr)
		{
			int idx = 0;
			for_every(dr, _dr)
			{
				dr->loadingOrder = idx;
				++idx;
			}
		}
	};

	/* rules are not prepared, not solved, they are used when loading (! not even resolving) */
	class DamageRuleSet
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(DamageRuleSet);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		DamageRuleSet(Framework::Library * _library, Framework::LibraryName const & _name);

		Array<DamageRule> const& get_rules() const { return rules; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~DamageRuleSet();

	private:
		Array<DamageRule> rules;
	};
};
