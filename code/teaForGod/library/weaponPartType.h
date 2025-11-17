#pragma once

#include "..\teaEnums.h"
#include "..\game\energy.h"
#include "..\game\weaponStatInfo.h"

#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\random\randomNumber.h"
#include "..\..\core\types\hand.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\text\localisedString.h"

namespace Framework
{
	class Actor;
	class ActorType;
	class Mesh;
	class MeshGenerator;
	class ModuleSoundData;
	class ModuleTemporaryObjectsData;

	namespace AI
	{
		class Mind;
	};
};

namespace TeaForGodEmperor
{
	class WeaponPart;
	struct TutorialHubId;

	namespace WeaponCoreKind
	{
		enum Type
		{
			None,
			Plasma,
			Ion,
			Corrosion,
			Lightning,
			MAX
		};

		tchar const * to_char(WeaponCoreKind::Type _coreKind);
		Name const & to_localised_string_id_icon(WeaponCoreKind::Type _coreKind);
		Name const & to_localised_string_id_short(WeaponCoreKind::Type _coreKind);
		Name const & to_localised_string_id_long(WeaponCoreKind::Type _coreKind);
		WeaponCoreKind::Type parse(String const& _text, WeaponCoreKind::Type _default);

		DamageType::Type to_damage_type(WeaponCoreKind::Type _coreKind);
		WeaponCoreKind::Type from_damage_type(DamageType::Type _damageType);
	};

	/**
	 *	Weapon Part
	 */
	class WeaponPartType
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(WeaponPartType);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		static void get_parts_tagged(TagCondition const& _tagged, OUT_ Array< WeaponPartType const*>& _weaponParts);

	public:
		enum Type
		{
			Unknown,
			GunChassis, // main element, has to be chosen and only then other parts may be attached, determines chamber size and chamber parameters in general (damage, speed)
			GunCore, // required, determines type of projectile
			GunMuzzle, // optional, but greatly alters how gun behaves, accuracy, speed, damage
			GunNode, // optional, may modify other parameters
			NUM
		};

		static Type parse_type(String const& _text, Type _default);

		struct Slot
		{
			Name id; // internal identification for slots, unique, works as mesh include stack
			Name connector; // internal identification - plug's and slot's id have to match
			WeaponPartType::Type connectType = WeaponPartType::Unknown; // as above - both have to match

			String const& get_ui_name() const;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc, String const& _locStrSubId);
		};

	public:
		WeaponPartType(Framework::Library * _library, Framework::LibraryName const & _name);

		Type get_type() const { return type; }
		bool can_be_chosen_randomly() const { return canBeChosenRandomly; }

		Name const& get_dedicated_schematic() const { return dedicatedSchematic; }

		Array<Framework::UsedLibraryStored<Framework::MeshGenerator>> const & get_mesh_generators() const { return meshGenerators; }
		Framework::MeshGenerator * get_mesh_generator(int _randomNumber) const { return meshGenerators.is_empty()? nullptr : meshGenerators[mod(_randomNumber, meshGenerators.get_size())].get(); }

		Framework::ModuleSoundData* get_sound_data() const { return soundData; }
		Framework::ModuleTemporaryObjectsData* get_temporary_objects_data() const { return temporaryObjectsData; }

		Slot const& get_plug() const { return plug; }
		Array<Slot> const& get_slots() const { return slots; }
		int get_slot_count(Name const& _connector) const;
		int get_slot_count(WeaponPartType::Type _byType) const;

	public:
		TutorialHubId get_tutorial_hub_id() const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~WeaponPartType();

	private:
		Type type = Type::Unknown;

		Array<Framework::UsedLibraryStored<Framework::MeshGenerator>> meshGenerators; // this is a part that is used when including this part to actual mesh (via  MeshGenElement IncludeStack)

		struct MeshGenerationParameters
		: public RefCountObject
		{
			bool always = false;
			bool isDefault = false;
			float probCoef = 1.0f;
			SimpleVariableStorage meshGenerationParameters; // these parameters affect all mesh generators (there has to be at least one)
			Array<RefCountObjectPtr<MeshGenerationParameters>> options;
			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		
			void process_whole_mesh_generation_parameters(Random::Generator const& _rg, bool _preferDesignatedDefault, std::function<void(SimpleVariableStorage const &, Random::Generator const& _rg)> _use) const;
		};
		RefCountObjectPtr<MeshGenerationParameters> wholeMeshGenerationParameters; // these parameters affect all mesh generators (there has to be at least one)
		int wholeMeshGenerationParametersPriority = 0; // if higher priority, overrides parameters

		Slot plug; // connector invalid for the main part
		Array<Slot> slots;

		Name dedicatedSchematic;

		bool canBeChosenRandomly = true;

		// gun
		WeaponPartTypeStatInfo<Optional<WeaponCoreKind::Type>> coreKind;
		// coefs are zero based
		//	+ sums		* mul/adj		^ max/top		= replaces
		Array<Colour> projectileColours; //													[=C/   /  /  /  ]
		WeaponPartTypeStatInfo<Random::Number<Energy>> chamber; //													[+C/+Ch/  /  /  ]
		WeaponPartTypeStatInfo<Random::Number<EnergyCoef>> chamberCoef; //											[+C/+Ch/  /+N/  ]		applied at the end
		WeaponPartTypeStatInfo<Random::Number<Energy>> baseDamageBoost; //											[  /   /  /+N/  ]		added before damage coef multiplies
		WeaponPartTypeStatInfo<Random::Number<EnergyCoef>> damageCoef; //											[+C/+Ch/+M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<Energy>> damageBoost; //												[+C/+Ch/+M/+N/  ]		added at the end
		WeaponPartTypeStatInfo<Random::Number<EnergyCoef>> armourPiercing; //										[+C/+Ch/+M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<Energy>> storage; //													[  /+Ch/  /+N/  ]
		WeaponPartTypeStatInfo<Random::Number<Energy>> storageOutputSpeed; //										[  /^Ch/  /^N/  ]
		WeaponPartTypeStatInfo<Random::Number<Energy>> storageOutputSpeedAdd; //									[  /   /  /+N/  ]
		WeaponPartTypeStatInfo<Random::Number<EnergyCoef>> storageOutputSpeedAdj; //								[  /   /  /*N/  ]		100% left as it is, applied post speed and add
		WeaponPartTypeStatInfo<Random::Number<Energy>> magazine; //													[  /+Ch/  /+N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> magazineCooldown; //											[+C/+Ch/  /+N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> magazineCooldownCoef; //										[  /+Ch/  /+N/  ]
		WeaponPartTypeStatInfo<Random::Number<Energy>> magazineOutputSpeed; //										[  /^Ch/  /^N/  ]
		WeaponPartTypeStatInfo<Random::Number<Energy>> magazineOutputSpeedAdd; //									[  /   /  /+N/  ]
		WeaponPartTypeStatInfo<Optional<bool>> continuousFire; //													[  /=Ch/  /=N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> projectileSpeed; //											[  /=Ch/=M/+N/  ]		M replaces Ch, but N can alter
		WeaponPartTypeStatInfo<Random::Number<float>> projectileSpread; // muzzle overrides chassis (in angles)		[  /=Ch/=M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> projectileSpeedCoef; // used only by core as a general--		[  /   /  /+N/+C]		N is applied first, C at the end
		WeaponPartTypeStatInfo<Random::Number<float>> projectileSpreadCoef; // --modification for kind				[  /   /  /+N/+C]
		WeaponPartTypeStatInfo<Random::Number<int>> numberOfProjectiles; // muzzle overrides chassis				[  /=Ch/=M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> antiDeflection; //											[+C/+Ch/+M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> maxDistCoef; //												[+C/+Ch/+M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> kineticEnergyCoef; //											[+C/+Ch/+M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<float>> confuse; //													[+C/+Ch/+M/+N/  ]
		WeaponPartTypeStatInfo<Random::Number<EnergyCoef>> mediCoef; //												[+C/+Ch/+M/+N/  ]
		
		ArrayStatic<Name, 8> specialFeatures;

		Framework::LocalisedString statInfo;
		Framework::LocalisedString partOnlyStatInfo; // displays only at part info (if set, won't show statInfo at part)

		Framework::ModuleSoundData* soundData = nullptr; // uses modules setup to provide stuff
		Framework::ModuleTemporaryObjectsData* temporaryObjectsData = nullptr; // uses modules setup to provide stuff

		void process_whole_mesh_generation_parameters(Random::Generator const& _rg, bool _preferDesignatedDefault, std::function<void(SimpleVariableStorage const&, Random::Generator const & _rg)> _use) const;

		friend class WeaponPart;
	};
};
