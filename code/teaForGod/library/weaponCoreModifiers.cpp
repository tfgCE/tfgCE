#include "weaponCoreModifiers.h"

#include "..\teaForGod.h"
#include "..\utils.h"
#include "..\game\gameSettings.h"
#include "..\game\playerSetup.h"
#include "..\game\weaponStatInfoImpl.inl"
#include "..\utils\buildStatsInfo.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\object\itemType.h"
#include "..\..\framework\object\temporaryObjectType.h"
#include "..\..\framework\text\stringFormatter.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// localised strings
DEFINE_STATIC_NAME_STR(lsCharsDamage, TXT("chars; damage"));
DEFINE_STATIC_NAME_STR(lsCharsAmmo, TXT("chars; ammo"));
DEFINE_STATIC_NAME_STR(lsCharsHealth, TXT("chars; health"));
//
	DEFINE_STATIC_NAME(coef);
	DEFINE_STATIC_NAME(cap);
	DEFINE_STATIC_NAME(part);
	DEFINE_STATIC_NAME(damage);
	DEFINE_STATIC_NAME(add);
	DEFINE_STATIC_NAME(value);
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayCostCoef, TXT("we.co.mo; ov.st; cost coef"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayDamageCoef, TXT("we.co.mo; ov.st; dmg coef"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayOutputSpeedCoef, TXT("we.co.mo; ov.st; output speed coef"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayProjectileSpeedCoef, TXT("we.co.mo; ov.st; projectile speed coef"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayProjectileSpreadAdd, TXT("we.co.mo; ov.st; projectile spread add"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayArmourPiercing, TXT("we.co.mo; ov.st; armour piercing"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayAntiDeflection, TXT("we.co.mo; ov.st; anti deflection"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayMaxDist, TXT("we.co.mo; ov.st; max dist"));
DEFINE_STATIC_NAME_STR(lsCoreModifierOverlayNoHitBoneDamage, TXT("we.co.mo; ov.st; no hit bone damage"));

//

REGISTER_FOR_FAST_CAST(WeaponCoreModifiers);
LIBRARY_STORED_DEFINE_TYPE(WeaponCoreModifiers, weaponCoreModifiers);

WeaponCoreModifiers::WeaponCoreModifiers(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
	SET_EXTRA_DEBUG_INFO(forCore, TXT("WeaponCoreModifiers.forCore"));
	forCore.set_size(WeaponCoreKind::MAX);
}

WeaponCoreModifiers::~WeaponCoreModifiers()
{
}

#define LOAD(what, set_auto_fn) \
	{ \
		fc.what.load_from_xml(node, TXT(#what)); \
		fc.what.set_auto_fn(); \
	}

bool WeaponCoreModifiers::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("for")))
	{
		if (auto* attr = node->get_attribute(TXT("coreKind")))
		{
			WeaponCoreKind::Type coreKind = WeaponCoreKind::parse(attr->get_as_string(), WeaponCoreKind::MAX);
			if (forCore.is_index_valid(coreKind))
			{
				auto& fc = forCore[coreKind];
				fc.overlayInfoColour.load_from_xml(node, TXT("overlayInfoColour"));
				LOAD(chamberSizeAdj, set_auto_coef_more_than_1_is_better);
				LOAD(damageAdj, set_auto_coef_more_than_1_is_better);
				LOAD(continuousDamageTime, set_auto_set);
				LOAD(continuousDamageMinTime, set_auto_set);
				LOAD(outputSpeedAdj, set_auto_coef_more_than_1_is_better);
				LOAD(projectileSpeedAdj, set_auto_coef_more_than_1_is_better);
				LOAD(projectileSpreadAdd, set_auto_positive_is_better);
				LOAD(armourPiercing, set_auto_positive_is_better);
				LOAD(antiDeflection, set_auto_positive_is_better);
				LOAD(maxDist, set_auto_set);
				fc.additionalWeaponInfo.load_from_xml_child(node, TXT("additionalWeaponInfo"), String::printf(TXT("%S; additionalWeaponInfo"), WeaponCoreKind::to_char(coreKind)).to_char(), false);
				fc.additionalCoreInfo.load_from_xml_child(node, TXT("additionalCoreInfo"), String::printf(TXT("%S; additionalCoreInfo"), WeaponCoreKind::to_char(coreKind)).to_char(), false);
				fc.sightColourSameAsProjectile = node->get_bool_attribute_or_from_child_presence(TXT("sightColourSameAsProjectile"), fc.sightColourSameAsProjectile);
				fc.noHitBoneDamage = node->get_bool_attribute_or_from_child_presence(TXT("noHitBoneDamage"), fc.noHitBoneDamage);
				fc.noContinuousFire = node->get_bool_attribute_or_from_child_presence(TXT("noContinuousFire"), fc.noContinuousFire);
				fc.singleSpecialProjectile = node->get_bool_attribute_or_from_child_presence(TXT("singleSpecialProjectile"), fc.singleSpecialProjectile);
				String subId = String::printf(TXT("projectile info; %S"), WeaponCoreKind::to_char(coreKind));
				fc.projectileInfo.load_from_xml_child(_node, TXT("projectileInfo"), _lc, subId.to_char());
			}
		}
	}

	return result;
}

void WeaponCoreModifiers::build_overlay_stats_info(WeaponCoreKind::Type _coreKind, List<String> & _list, String const& _linePrefix) const
{
	if (auto* fc = get_for_core(_coreKind))
	{
		EnergyCoef damageCoef = EnergyCoef::one();
		EnergyCoef costCoef = EnergyCoef::one();
		{
			auto v = fc->chamberSizeAdj;
			if (v.value.is_set())
			{
				damageCoef = damageCoef.adjusted(v.value.get());
				costCoef = costCoef.adjusted(v.value.get());
			}
		}
		{
			auto v = fc->damageAdj;
			if (v.value.is_set())
			{
				damageCoef = damageCoef.adjusted(v.value.get());
			}
		}

		if (!damageCoef.is_one())
		{
			_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayDamageCoef), Framework::StringFormatterParams()
				.add(NAME(coef), damageCoef.as_string_percentage(0))));
		}
		if (!costCoef.is_one())
		{
			_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayCostCoef), Framework::StringFormatterParams()
				.add(NAME(coef), costCoef.as_string_percentage(0))));
		}
		if (fc->outputSpeedAdj.value.is_set())
		{
			_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayOutputSpeedCoef), Framework::StringFormatterParams()
				.add(NAME(coef), fc->outputSpeedAdj.value.get().as_string_percentage(0))));
		}

#ifdef GUN_STATS__SHOW__NO_HIT_BONE_DAMAGE 
		{
			if (fc->noHitBoneDamage)
			{
				_list.push_back(_linePrefix + LOC_STR(NAME(lsCoreModifierOverlayNoHitBoneDamage)));
			}
		}
#endif
		{
			auto v = fc->maxDist;
			if (v.value.is_set())
			{
				if (auto* ps = TeaForGodEmperor::PlayerSetup::access_current_if_exists())
				{
					String str;
					auto ms = ps->get_preferences().get_measurement_system();
					if (ms == MeasurementSystem::Imperial)
					{
						str = MeasurementSystem::as_feet_inches(v.value.get());
					}
					else
					{
						str = MeasurementSystem::as_meters(v.value.get());
					}
					_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayMaxDist), Framework::StringFormatterParams()
						.add(NAME(value), str)));
				}

			}
		} 
		{
			auto v = fc->armourPiercing;
			if (v.value.is_set())
			{
				_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayArmourPiercing), Framework::StringFormatterParams()
					.add(NAME(coef), v.value.get().as_string_percentage(0))));
			}
		}
		{
			auto v = fc->antiDeflection;
			if (v.value.is_set())
			{
				_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayAntiDeflection), Framework::StringFormatterParams()
					.add(NAME(coef), String::printf(TXT("%.0f%%"), v.value.get() * 100.0f))));
			}
		}

		////////////////////////////////////////////////

		{
			auto v = fc->projectileSpeedAdj;
			if (v.value.is_set() && v.value.get() != 1.0f)
			{
				_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayProjectileSpeedCoef), Framework::StringFormatterParams()
					.add(NAME(coef), String::printf(TXT("%.0f%%"), 100.0f * v.value.get()))));
			}
		}
		if (fc->projectileSpreadAdd.value.is_set())
		{
			String value;
			{
				float spread = fc->projectileSpreadAdd.value.get();
				String symbol(spread >= 0.0f ? TXT("+") : TXT("-"));
				value = symbol + String::printf(TXT("%.0f%S"), abs(spread), String::degree().to_char());
			}
			_list.push_back(_linePrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsCoreModifierOverlayProjectileSpreadAdd), Framework::StringFormatterParams()
				.add(NAME(add), value)));
		}

		if (fc->additionalWeaponInfo.is_valid())
		{
			String str = fc->additionalWeaponInfo.get();
			if (!str.is_empty())
			{
				List<String> lines;
				str.split(String(TXT("~")), lines);
				for_every(line, lines)
				{
					_list.push_back(_linePrefix + *line);
				}
			}
		}

		if (fc->additionalCoreInfo.is_valid())
		{
			String str = fc->additionalCoreInfo.get();
			if (!str.is_empty())
			{
				List<String> lines;
				str.split(String(TXT("~")), lines);
				for_every(line, lines)
				{
					_list.push_back(_linePrefix + *line);
				}
			}
		}

		Utils::split_multi_lines(REF_ _list);
	}
}
