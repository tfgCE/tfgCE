#include "damageRules.h"

#include "library.h"

#include "..\game\game.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

void HealthExtraEffectParams::setup_formatter_params(REF_ Framework::StringFormatterParams& _params) const
{
	if (investigatorInfo.provideArmourPiercingAs.is_set() &&
		armourPiercing.is_set())
	{
		_params.add(investigatorInfo.provideArmourPiercingAs.get(), armourPiercing.get().as_string_percentage_auto_decimals());
	}
	if (investigatorInfo.provideArmourPiercingArmourAs.is_set() &&
		armourPiercing.is_set())
	{
		_params.add(investigatorInfo.provideArmourPiercingArmourAs.get(), (EnergyCoef::one() - armourPiercing.get()).as_string_percentage_auto_decimals());
	}
	if (investigatorInfo.provideDamageCoefAs.is_set() &&
		damageCoef.is_set())
	{
		_params.add(investigatorInfo.provideDamageCoefAs.get(), damageCoef.get().as_string_percentage_auto_decimals());
	}
	if (investigatorInfo.provideExtraDamageCapAs.is_set() &&
		extraDamageCap.is_set())
	{
		_params.add(investigatorInfo.provideExtraDamageCapAs.get(), extraDamageCap.get().as_string_auto_decimals());
	}
}

//
 
bool HealthExtraEffect::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	time.load_from_xml(_node, TXT("time"));
	if (auto* attr = _node->get_attribute(TXT("forContinuousDamageType")))
	{
		forContinuousDamageType = DamageType::parse(attr->get_as_string());
		if (forContinuousDamageType.is_set() && forContinuousDamageType.get() == DamageType::Unknown)
		{
			forContinuousDamageType.clear();
		}
	}
	temporaryObject = _node->get_name_attribute(TXT("temporaryObject"), temporaryObject);
	temporaryObjectType.load_from_xml(_node, TXT("temporaryObjectType"), _lc);
	
	if (auto* attr = _node->get_attribute(TXT("ignoreForDamageType")))
	{
		ignoreForDamageType = DamageType::parse(attr->get_as_string());
		if (ignoreForDamageType.is_set() && ignoreForDamageType.get() == DamageType::Unknown)
		{
			ignoreForDamageType.clear();
		}
	}

	damageCoef.load_from_xml(_node, TXT("damageCoef"));
	extraDamageCap.load_from_xml(_node, TXT("extraDamageCap"));
	ricocheting.load_from_xml(_node, TXT("ricocheting"));
	armourPiercing.load_from_xml(_node, TXT("armourPiercing"));

	globalTintColour.load_from_xml(_node, TXT("globalTintColour"));

	for_every(node, _node->children_named(TXT("info")))
	{
		investigatorInfo = InvestigatorInfo();
		investigatorInfo.id = node->get_name_attribute(TXT("id"), investigatorInfo.id);
		error_loading_xml_on_assert(investigatorInfo.id.is_valid(), result, node, TXT("no \"id\" provided"));
		
		investigatorInfo.provideDamageCoefAs.load_from_xml(node, TXT("provideDamageCoefAs"));
		investigatorInfo.provideExtraDamageCapAs.load_from_xml(node, TXT("provideExtraDamageCapAs"));
		investigatorInfo.provideArmourPiercingAs.load_from_xml(node, TXT("provideArmourPiercingAs"));
		investigatorInfo.provideArmourPiercingArmourAs.load_from_xml(node, TXT("provideArmourPiercingArmourAs"));

		investigatorInfo.icon.load_from_xml_child(node, TXT("icon"), _lc, String::printf(TXT("%S; icon"), investigatorInfo.id.to_char()).to_char());
		investigatorInfo.info.load_from_xml_child(node, TXT("info"), _lc, String::printf(TXT("%S; info"), investigatorInfo.id.to_char()).to_char());
	}

	return result;
}

bool HealthExtraEffect::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= temporaryObjectType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool DamageRule::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("missing \"id\" for a damage rule"));

	forDamageType.load_from_xml(_node, TXT("forDamageType"));
	forDamageExtraInfo.load_from_xml(_node, TXT("forDamageExtraInfo"));
	forMeleeDamage.load_from_xml(_node, TXT("forMeleeDamage"));
	forExplosionDamage.load_from_xml(_node, TXT("forExplosionDamage"));
	ifCurrentExtraEffect.load_from_xml(_node, TXT("ifCurrentExtraEffect"));
	ifCurrentContinuousDamageType.load_from_xml(_node, TXT("ifCurrentContinuousDamageType"));
	forDamagerIsExplosion.load_from_xml(_node, TXT("forDamagerIsExplosion"));
	forDamagerIsAlly.load_from_xml(_node, TXT("forDamagerIsAlly"));
	forDamagerIsSelf.load_from_xml(_node, TXT("forDamagerIsSelf"));

	kill = _node->get_bool_attribute_or_from_child_presence(TXT("kill"), kill);
	heal = _node->get_bool_attribute_or_from_child_presence(TXT("heal"), heal);
	noDamage = _node->get_bool_attribute_or_from_child_presence(TXT("noDamage"), noDamage);
	noContinuousDamage = _node->get_bool_attribute_or_from_child_presence(TXT("noContinuousDamage"), noContinuousDamage);
	noExplosion = _node->get_bool_attribute_or_from_child_presence(TXT("noExplosion"), noExplosion);
	addToProjectileDamage = _node->get_bool_attribute_or_from_child_presence(TXT("addToProjectileDamage"), addToProjectileDamage);

	applyCoef.load_from_xml(_node, TXT("applyCoef"));

	endExtraEffect.load_from_xml(_node, TXT("endExtraEffect"));
	endContinuousDamageType.load_from_xml(_node, TXT("endContinuousDamageType"));
	spawnTemporaryObjectCooldown.load_from_xml(_node, TXT("spawnTemporaryObjectCooldown"));
	spawnTemporaryObjectCooldownId = _node->get_name_attribute_or_from_child(TXT("spawnTemporaryObjectCooldownId"), spawnTemporaryObjectCooldownId);
	spawnTemporaryObjectAtDamager = _node->get_bool_attribute_or_from_child_presence(TXT("spawnTemporaryObjectAtDamager"), spawnTemporaryObjectAtDamager);
	spawnTemporaryObject = _node->get_name_attribute_or_from_child(TXT("spawnTemporaryObject"), spawnTemporaryObject);
	spawnTemporaryObjectType.load_from_xml(_node, TXT("spawnTemporaryObjectType"));
	temporaryObjectDamageCoef.load_from_xml(_node, TXT("temporaryObjectDamageCoef"));
	for_every(node, _node->children_named(TXT("spawn")))
	{
		spawnTemporaryObjectCooldown.load_from_xml(node, TXT("cooldown"));
		spawnTemporaryObjectCooldownId = node->get_name_attribute(TXT("cooldownId"), spawnTemporaryObjectCooldownId);
		spawnTemporaryObjectAtDamager = node->get_bool_attribute_or_from_child_presence(TXT("atDamager"), spawnTemporaryObjectAtDamager);
		spawnTemporaryObject = node->get_name_attribute(TXT("temporaryObject"), spawnTemporaryObject);
		spawnTemporaryObjectType.load_from_xml(node, TXT("temporaryObjectType"));
		temporaryObjectDamageCoef.load_from_xml(node, TXT("damageCoef"));
	}

	extraEffects.clear();
	for_every(node, _node->children_named(TXT("extraEffect")))
	{
		HealthExtraEffect ee;
		if (ee.load_from_xml(node, _lc))
		{
			extraEffects.push_back(ee);
		}
		else
		{
			result = false;
		}
	}

	error_loading_xml_on_assert(!spawnTemporaryObjectCooldown.is_set() || spawnTemporaryObjectCooldownId.is_valid(), result, _node, TXT("cooldown should be accompanied by cooldownId"));

	stopProcessing = _node->get_bool_attribute_or_from_child_presence(TXT("stopProcessing"), stopProcessing);

	return result;
}

bool DamageRule::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= spawnTemporaryObjectType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	for_every(ee, extraEffects)
	{
		result &= ee->prepare_for_game(_library, _pfgContext);
	}

	return result;
}


//

REGISTER_FOR_FAST_CAST(DamageRuleSet);
LIBRARY_STORED_DEFINE_TYPE(DamageRuleSet, damageRuleSet);

DamageRuleSet::DamageRuleSet(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

DamageRuleSet::~DamageRuleSet()
{
}

bool DamageRuleSet::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("damageRule")))
	{
		DamageRule dr;
		if (dr.load_from_xml(node, _lc))
		{
			rules.push_back(dr);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool DamageRuleSet::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	
	// not actually prepared

	return result;
}
