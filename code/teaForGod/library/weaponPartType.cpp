#include "weaponPartType.h"

#include "library.h"

#include "..\game\weaponStatInfoImpl.inl"

#include "..\tutorials\tutorialHubId.h"

#include "..\..\core\random\randomUtils.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define INVESTIGATE_WEAPON_PART_SETUP
#endif

//

using namespace TeaForGodEmperor;

//

// localised strings
DEFINE_STATIC_NAME_STR(lsIconNone, TXT("chars; weapon core; none"));
DEFINE_STATIC_NAME_STR(lsIconPlasma, TXT("chars; weapon core; plasma"));
DEFINE_STATIC_NAME_STR(lsIconIon, TXT("chars; weapon core; ion"));
DEFINE_STATIC_NAME_STR(lsIconCorrosion, TXT("chars; weapon core; corrosion"));
DEFINE_STATIC_NAME_STR(lsIconLighting, TXT("chars; weapon core; lightning"));
//
DEFINE_STATIC_NAME_STR(lsShortNone, TXT("weapon part; core; none"));
DEFINE_STATIC_NAME_STR(lsShortPlasma, TXT("weapon part; core; plasma"));
DEFINE_STATIC_NAME_STR(lsShortIon, TXT("weapon part; core; ion"));
DEFINE_STATIC_NAME_STR(lsShortCorrosion, TXT("weapon part; core; corrosion"));
DEFINE_STATIC_NAME_STR(lsShortLighting, TXT("weapon part; core; lightning"));
//
DEFINE_STATIC_NAME_STR(lsLongNone, TXT("weapon part stats; core; none"));
DEFINE_STATIC_NAME_STR(lsLongPlasma, TXT("weapon part stats; core; plasma"));
DEFINE_STATIC_NAME_STR(lsLongIon, TXT("weapon part stats; core; ion"));
DEFINE_STATIC_NAME_STR(lsLongCorrosion, TXT("weapon part stats; core; corrosion"));
DEFINE_STATIC_NAME_STR(lsLongLighting, TXT("weapon part stats; core; lightning"));
//
DEFINE_STATIC_NAME_STR(lsSlotGunCore, TXT("weapon part; slot; gun core"));
DEFINE_STATIC_NAME_STR(lsSlotGunMuzzle, TXT("weapon part; slot; gun muzzle"));
DEFINE_STATIC_NAME_STR(lsSlotGunNode, TXT("weapon part; slot; gun node"));
DEFINE_STATIC_NAME_STR(lsSlotGunChassis, TXT("weapon part; slot; gun chassis"));

//

tchar const* WeaponCoreKind::to_char(WeaponCoreKind::Type _coreKind)
{
	if (_coreKind == None) return TXT("none");
	if (_coreKind == Plasma) return TXT("plasma");
	if (_coreKind == Ion) return TXT("ion");
	if (_coreKind == Corrosion) return TXT("corrosion");
	if (_coreKind == Lightning) return TXT("lightning");
	todo_implement;
	return TXT("??");
}

Name const& WeaponCoreKind::to_localised_string_id_icon(WeaponCoreKind::Type _coreKind)
{
	if (_coreKind == None) return NAME(lsIconNone);
	if (_coreKind == Plasma) return NAME(lsIconPlasma);
	if (_coreKind == Ion) return NAME(lsIconIon);
	if (_coreKind == Corrosion) return NAME(lsIconCorrosion);
	if (_coreKind == Lightning) return NAME(lsIconLighting);
	todo_implement;
	return Name::invalid();
}

Name const& WeaponCoreKind::to_localised_string_id_short(WeaponCoreKind::Type _coreKind)
{
	if (_coreKind == None) return NAME(lsShortNone);
	if (_coreKind == Plasma) return NAME(lsShortPlasma);
	if (_coreKind == Ion) return NAME(lsShortIon);
	if (_coreKind == Corrosion) return NAME(lsShortCorrosion);
	if (_coreKind == Lightning) return NAME(lsShortLighting);
	todo_implement;
	return Name::invalid();
}

Name const& WeaponCoreKind::to_localised_string_id_long(WeaponCoreKind::Type _coreKind)
{
	if (_coreKind == None) return NAME(lsLongNone);
	if (_coreKind == Plasma) return NAME(lsLongPlasma);
	if (_coreKind == Ion) return NAME(lsLongIon);
	if (_coreKind == Corrosion) return NAME(lsLongCorrosion);
	if (_coreKind == Lightning) return NAME(lsLongLighting);
	todo_implement;
	return Name::invalid();
}

WeaponCoreKind::Type WeaponCoreKind::parse(String const& _text, WeaponCoreKind::Type _default)
{
	if (_text.is_empty()) return _default;
	if (_text == TXT("none")) return None;
	if (_text == TXT("plasma")) return Plasma;
	if (_text == TXT("ion")) return Ion;
	if (_text == TXT("corrosion")) return Corrosion;
	if (_text == TXT("lightning")) return Lightning;

	error(TXT("weapon core kind \"%S\" not recognised"), _text.to_char());
	return _default;
}

DamageType::Type WeaponCoreKind::to_damage_type(WeaponCoreKind::Type _coreKind)
{
	if (_coreKind == Plasma) return DamageType::Plasma;
	if (_coreKind == Ion) return DamageType::Ion;
	if (_coreKind == Corrosion) return DamageType::Corrosion;
	if (_coreKind == Lightning) return DamageType::Lightning;
	return DamageType::Unknown;
}

WeaponCoreKind::Type WeaponCoreKind::from_damage_type(DamageType::Type _damageType)
{
	if (_damageType == DamageType::Plasma) return Plasma;
	if (_damageType == DamageType::Ion) return Ion;
	if (_damageType == DamageType::Corrosion) return Corrosion;
	if (_damageType == DamageType::Lightning) return Lightning;
	return None;
}

//--

bool WeaponPartType::MeshGenerationParameters::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	if (_node->get_name() == TXT("choose"))
	{
		always = false;
		isDefault = _node->get_bool_attribute_or_from_child_presence(TXT("default"), false);
		probCoef = _node->get_float_attribute(TXT("probCoef"), probCoef);
	}
	else
	{
		isDefault = false;
		always = true;
	}

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("params") ||
			node->get_name() == TXT("parameters"))
		{
			result &= meshGenerationParameters.load_from_xml(node);
		}
		if (node->get_name() == TXT("always") ||
			node->get_name() == TXT("choose"))
		{
			RefCountObjectPtr<MeshGenerationParameters> mgp;
			mgp = new MeshGenerationParameters();
			if (mgp->load_from_xml(node, _lc))
			{
				options.push_back(mgp);
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

void WeaponPartType::MeshGenerationParameters::process_whole_mesh_generation_parameters(Random::Generator const& _rg, bool _preferDesignatedDefault, std::function<void(SimpleVariableStorage const&, Random::Generator const& _rg)> _use) const
{
#ifdef INVESTIGATE_WEAPON_PART_SETUP
	output(TXT("  %c%c%i rg = %S"), always? 'A' : '-', isDefault? 'D' : '-', options.get_size(), _rg.get_seed_string().to_char());
#endif
	{
		Random::Generator rg = _rg;
		rg.advance_seed(239, 9752);
		_use(meshGenerationParameters, rg);
	}

	if (options.is_empty())
	{
		return;
	}

	int useIndex = NONE;

	if (useIndex == NONE && _preferDesignatedDefault)
	{
		for_every_ref(opt, options)
		{
			if (!opt->always && opt->isDefault)
			{
				useIndex = for_everys_index(opt);
				break;
			}
		}
	}

	if (useIndex == NONE)
	{
		struct Choose
		{
			int idx;
			float probCoef;
			Choose() {}
			Choose(int _idx, float _probCoef) : idx(_idx), probCoef(_probCoef) {}
		};

		ARRAY_STACK(Choose, choose, options.get_size());
		for_every_ref(opt, options)
		{
			if (!opt->always)
			{
				choose.push_back(Choose(for_everys_index(opt), opt->probCoef));
			}
		}

		if (!choose.is_empty())
		{
			Random::Generator rg = _rg;
			rg.advance_seed(234, 959475);

			int useChooseIndex = RandomUtils::ChooseFromContainer<ArrayStack<Choose>, Choose>::choose(
				rg, choose, [](Choose const& _e) { return _e.probCoef; });
			if (useChooseIndex != NONE)
			{
				useIndex = choose[useChooseIndex].idx;
#ifdef INVESTIGATE_WEAPON_PART_SETUP
				output(TXT("   chosen index %i (%i) (rg:%S)"), useIndex, useChooseIndex, rg.get_seed_string().to_char());
#endif
			}
		}
	}

	for_every_ref(opt, options)
	{
		int idx = for_everys_index(opt);
		Random::Generator rg = _rg;
		rg.advance_seed(852 + 2967 * idx, 964 + 13 * idx);
		if (opt->always || useIndex == idx)
		{
#ifdef INVESTIGATE_WEAPON_PART_SETUP
			output(TXT("   do %S %i/%i"), opt->always? TXT("always") : TXT("chosen"), for_everys_index(opt), options.get_size());
#endif
			opt->process_whole_mesh_generation_parameters(rg, _preferDesignatedDefault, _use);
		}
	}
}

//--

REGISTER_FOR_FAST_CAST(WeaponPartType);
LIBRARY_STORED_DEFINE_TYPE(WeaponPartType, weaponPartType);

void WeaponPartType::get_parts_tagged(TagCondition const& _tagged, OUT_ Array< WeaponPartType const*>& _weaponParts)
{
	for_every_ptr(wpt, Library::get_current_as<Library>()->get_weapon_part_types().get_tagged(_tagged))
	{
		_weaponParts.push_back(wpt);
	}
}

WeaponPartType::WeaponPartType(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
	SET_EXTRA_DEBUG_INFO(specialFeatures, TXT("WeaponPartType.specialFeatures"));
}

WeaponPartType::~WeaponPartType()
{
	delete_and_clear(soundData);
	delete_and_clear(temporaryObjectsData);
}

WeaponPartType::Type WeaponPartType::parse_type(String const& _text, Type _default)
{
	if (_text.is_empty()) return _default;
	if (_text == TXT("gun chassis")) return GunChassis;
	if (_text == TXT("gun core")) return GunCore;
	if (_text == TXT("gun muzzle")) return GunMuzzle;
	if (_text == TXT("gun node")) return GunNode;
	error(TXT("not recognised weapon part type \"%S\""), _text.to_char());
	return Unknown;
}

#define LOAD(what, set_auto_fn) \
	{ \
		if (node->has_attribute(TXT(#what))) { warn_loading_xml(node, TXT("weapon part type stats as attributes are deprecated, use child nodes")); } \
		result &= what.value.load_from_xml(node, TXT(#what)); \
		if (auto* n = node->first_child_named(TXT(#what))) \
		{ \
			if (auto* a = n->get_attribute(TXT("affects"))) \
			{ \
				what.how = WeaponStatAffection::parse(a->get_as_string()); \
			} \
		} \
		what.set_auto_fn(); \
	}

bool WeaponPartType::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	dedicatedSchematic = _node->get_name_attribute_or_from_child(TXT("dedicatedSchematic"), dedicatedSchematic);

	canBeChosenRandomly = _node->get_bool_attribute_or_from_child_presence(TXT("canBeChosenRandomly"), canBeChosenRandomly);
	canBeChosenRandomly = ! _node->get_bool_attribute_or_from_child_presence(TXT("blockChoosingRandomly"), ! canBeChosenRandomly);

	for_every(node, _node->children_named(TXT("sound")))
	{
		if (!soundData)
		{
			soundData = new Framework::ModuleSoundData(this);
		}
		result &= soundData->load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("temporaryObjects")))
	{
		if (!temporaryObjectsData)
		{
			temporaryObjectsData = new Framework::ModuleTemporaryObjectsData(this);
		}
		result &= temporaryObjectsData->load_from_xml(node, _lc);
	}

	type = parse_type(_node->get_string_attribute(TXT("type")), type);
	error_loading_xml_on_assert(type != Unknown, result, _node, TXT("not defined type"));

	meshGenerators.clear();

	if (auto* attr = _node->get_attribute(TXT("meshGenerator")))
	{
		meshGenerators.set_size(meshGenerators.get_size() + 1);
		result &= meshGenerators.get_last().load_from_xml(attr, _lc);
	}
	for_every(node, _node->children_named(TXT("meshGenerators")))
	{
		for_every(mgNode, node->children_named(TXT("meshGenerator")))
		{
			meshGenerators.set_size(meshGenerators.get_size() + 1);
			result &= meshGenerators.get_last().load_from_xml(mgNode, _lc);
		}
	}
	error_loading_xml_on_assert(! _node->first_child_named(TXT("meshGenerationParameters")), result, _node, TXT("rename to \"wholeWeaponMeshGenerationParameters\""));
	for_every(node, _node->children_named(TXT("wholeWeaponMeshGenerationParameters")))
	{
		wholeMeshGenerationParametersPriority = node->get_int_attribute(TXT("priority"), wholeMeshGenerationParametersPriority);
		if (!wholeMeshGenerationParameters.is_set())
		{
			wholeMeshGenerationParameters = new MeshGenerationParameters();
		}
		result &= wholeMeshGenerationParameters->load_from_xml(node, _lc);
	}

	plug.connectType = type; // should match
	for_every(node, _node->children_named(TXT("plug")))
	{
		result &= plug.load_from_xml(node, _lc, String(TXT("plug")));
		error_loading_xml_on_assert(plug.connectType == type, result, node, TXT("plug type should match type!"));
	}

	for_every(slotsNode, _node->children_named(TXT("slots")))
	{
		for_every(node, slotsNode->children_named(TXT("slot")))
		{
			Slot slot;
			if (slot.load_from_xml(node, _lc, String::printf(TXT("slot %i"), slots.get_size())))
			{
				slots.push_back(slot);
			}
		}
	}

	// gun
	for_every(node, _node->children_named(TXT("gun")))
	{
		for_every(n, node->children_named(TXT("core")))
		{
			if (auto* a = n->get_attribute(TXT("kind")))
			{
				coreKind.value = WeaponCoreKind::parse(a->get_as_string(), WeaponCoreKind::Plasma);
				coreKind.how = WeaponStatAffection::Set;
			}
			if (auto* a = n->get_attribute(TXT("projectileColour")))
			{
				Colour projectileColour = Colour::white;
				projectileColour.parse_from_string(a->get_as_string());
				projectileColours.push_back(projectileColour);
			}
			for_every(cNode, n->children_named(TXT("projectileColour")))
			{
				Colour projectileColour = Colour::white;
				projectileColour.load_from_xml(cNode);
				projectileColours.push_back(projectileColour);
			}
		}

		LOAD(chamber,					set_auto_positive_is_better);
		LOAD(chamberCoef,				set_auto_positive_is_better);
		LOAD(baseDamageBoost,			set_auto_positive_is_better);
		LOAD(damageCoef,				set_auto_positive_is_better);
		LOAD(damageBoost,				set_auto_positive_is_better);
		LOAD(armourPiercing,			set_auto_positive_is_better);
		LOAD(storage,					set_auto_positive_is_better);
		LOAD(storageOutputSpeed,		set_auto_positive_is_better);
		LOAD(storageOutputSpeedAdd,		set_auto_positive_is_better);
		LOAD(storageOutputSpeedAdj,		set_auto_coef_more_than_1_is_better);
		LOAD(magazine,					set_auto_positive_is_better);
		LOAD(magazineCooldown,			set_auto_positive_is_better);
		LOAD(magazineCooldownCoef,		set_auto_positive_is_better);
		LOAD(magazineOutputSpeed,		set_auto_positive_is_better);
		LOAD(magazineOutputSpeedAdd,	set_auto_positive_is_better);
		LOAD(continuousFire,			set_auto_special);
		LOAD(projectileSpeed,			set_auto_positive_is_better);
		LOAD(projectileSpread,			set_auto_positive_is_better);
		LOAD(projectileSpeedCoef,		set_auto_positive_is_better);
		LOAD(projectileSpreadCoef,		set_auto_positive_is_better);
		LOAD(numberOfProjectiles,		set_auto_positive_is_better);
		LOAD(antiDeflection,			set_auto_positive_is_better);
		LOAD(maxDistCoef,				set_auto_positive_is_better);
		LOAD(kineticEnergyCoef,			set_auto_positive_is_better);
		LOAD(confuse,					set_auto_positive_is_better);
		LOAD(mediCoef,					set_auto_positive_is_better);

		{
			Name sf = node->get_name_attribute(TXT("specialFeature"));
			if (sf.is_valid())
			{
				specialFeatures.push_back_unique(sf);
			}
			for_every(n, node->children_named(TXT("specialFeature")))
			{
				Name sf = Name(n->get_internal_text());
				if (sf.is_valid())
				{
					specialFeatures.push_back_unique(sf);
				}
			}
		}

		statInfo.load_from_xml_child(node, TXT("statInfo"), _lc, TXT("statInfo"));
		partOnlyStatInfo.load_from_xml_child(node, TXT("partOnlyStatInfo"), _lc, TXT("partOnlyStatInfo"));
	}

	return result;
}

bool WeaponPartType::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(mg, meshGenerators)
	{
		result &= mg->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	if (soundData)
	{
		result &= soundData->prepare_for_game(_library, _pfgContext);
	}

	if (temporaryObjectsData)
	{
		result &= temporaryObjectsData->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

TutorialHubId WeaponPartType::get_tutorial_hub_id() const
{
	TutorialHubId tutorialHubId;
	tutorialHubId.set(String::printf(TXT("WEAPONPARTTYPE_%S:%S"), get_name().get_group().to_char(), get_name().get_name().to_char()));
	return tutorialHubId;
}

void WeaponPartType::process_whole_mesh_generation_parameters(Random::Generator const& _rg, bool _preferDesignatedDefault, std::function<void(SimpleVariableStorage const&, Random::Generator const& _rg)> _use) const
{
	if (!wholeMeshGenerationParameters.is_set())
	{
		return;
	}

#ifdef INVESTIGATE_WEAPON_PART_SETUP
	output(TXT("process_whole_mesh_generation_parameters \"%S\""), get_name().to_string().to_char());
#endif
	wholeMeshGenerationParameters->process_whole_mesh_generation_parameters(_rg, _preferDesignatedDefault, _use);
}

int WeaponPartType::get_slot_count(Name const& _connector) const
{
	int result = 0;
	for_every(slot, slots)
	{
		if (slot->connector == _connector)
		{
			++result;
		}
	}
	return result;
}

int WeaponPartType::get_slot_count(WeaponPartType::Type _byType) const
{
	int result = 0;
	for_every(slot, slots)
	{
		if (slot->connectType == _byType)
		{
			++result;
		}
	}
	return result;
}

//--

bool WeaponPartType::Slot::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc, String const& _locStrSubId)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	connector = _node->get_name_attribute(TXT("connector"), connector);
	connectType = parse_type(_node->get_string_attribute(TXT("type")), connectType);
	error_loading_xml_on_assert(connectType != Unknown, result, _node, TXT("provide a valid type of connector"));

	return result;
}

String const& WeaponPartType::Slot::get_ui_name() const
{
	if (connectType == WeaponPartType::GunCore) return LOC_STR(NAME(lsSlotGunCore));
	if (connectType == WeaponPartType::GunMuzzle) return LOC_STR(NAME(lsSlotGunMuzzle));
	if (connectType == WeaponPartType::GunNode) return LOC_STR(NAME(lsSlotGunNode));
	if (connectType == WeaponPartType::GunChassis) return LOC_STR(NAME(lsSlotGunChassis));
	todo_implement;
	return String::empty();
}