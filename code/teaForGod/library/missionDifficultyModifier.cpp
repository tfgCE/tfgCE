#include "missionDifficultyModifier.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\utils\reward.h"

#include "..\..\core\random\randomUtils.h"
#include "..\..\core\other\parserUtils.h"
#include "..\..\core\tags\tag.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\framework\meshGen\meshGenValueDefImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// localised strings
DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));

DEFINE_STATIC_NAME_STR(lsBonusInfo, TXT("mission difficulty modifier; bonus"));
DEFINE_STATIC_NAME_STR(lsOnEndBonus, TXT("mission difficulty modifier; bonus; on end"));
DEFINE_STATIC_NAME(bonusCoef);
DEFINE_STATIC_NAME_STR(lsOnSuccessBonus, TXT("mission difficulty modifier; bonus; on success"));
DEFINE_STATIC_NAME_STR(lsOnComeBackBonus, TXT("mission difficulty modifier; bonus; on come back"));
DEFINE_STATIC_NAME_STR(lsAppliedBonus, TXT("mission difficulty modifier; applied bonus"));

//

REGISTER_FOR_FAST_CAST(MissionDifficultyModifier);
LIBRARY_STORED_DEFINE_TYPE(MissionDifficultyModifier, missionDifficultyModifier);

MissionDifficultyModifier::MissionDifficultyModifier(Framework::Library* _library, Framework::LibraryName const& _name)
	: base(_library, _name)
{
}

MissionDifficultyModifier::~MissionDifficultyModifier()
{
}

bool MissionDifficultyModifier::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	typeDesc = Framework::LocalisedString();
	description = Framework::LocalisedString();
	onEndBonus = BonusCoef();
	onSuccessBonus = BonusCoef();
	onComeBackBonus = BonusCoef();
	forPool.clear();
	cellRegionTagCondition.clear();
	autoHostiles = GameDirectorDefinition::AutoHostiles();

	// load
	//
	typeDesc.load_from_xml_child(_node, TXT("typeDesc"), _lc, TXT("type desc"));
	description.load_from_xml_child(_node, TXT("description"), _lc, TXT("description"));
	for_every(n, _node->children_named(TXT("onEndBonus")))
	{
		result &= onEndBonus.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("onSuccessBonus")))
	{
		result &= onSuccessBonus.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("onComeBackBonus")))
	{
		result &= onComeBackBonus.load_from_xml(n);
	}

	for_every(n, _node->children_named(TXT("forPool")))
	{
		forPool.push_back();
		auto& fp = forPool.get_last();
		fp.pool = n->get_name_attribute_or_from_child(TXT("name"), fp.pool);
		fp.amountAdd.load_from_xml(n, TXT("amountAdd"));
		fp.amountCoef.load_from_xml(n, TXT("amountCoef"));
		fp.concurrentLimitCoef.load_from_xml(n, TXT("concurrentLimitCoef"));
	}
	
	cellRegionTagCondition.load_from_xml_attribute_or_child_node(_node, TXT("cellRegionTagCondition"));

	for_every(n, _node->children_named(TXT("autoHostiles")))
	{
		result &= autoHostiles.load_from_xml(n);
	}

	return result;
}

bool MissionDifficultyModifier::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

MissionDifficultyModifier::ForPool const* MissionDifficultyModifier::get_for_pool(Name const& _pool) const
{
	MissionDifficultyModifier::ForPool const* defFP = nullptr;
	for_every(fp, forPool)
	{
		if (fp->pool == _pool)
		{
			return fp;
		}
		if (!fp->pool.is_valid())
		{
			defFP = fp;
		}
	}
	return defFP;
}

String MissionDifficultyModifier::get_type_desc() const
{
	return typeDesc.get();
}

String MissionDifficultyModifier::get_description() const
{
	return description.get();
}

String MissionDifficultyModifier::get_bonus_info(bool _successPossible) const
{
	String result;

	if (!onEndBonus.is_empty())
	{
		if (!result.is_empty()) result += TXT("~");
		result += Framework::StringFormatter::format_loc_str(NAME(lsOnEndBonus), Framework::StringFormatterParams()
			.add(NAME(bonusCoef), onEndBonus.as_string()));
	}
	if (!onSuccessBonus.is_empty())
	{
		if (!result.is_empty()) result += TXT("~");
		result += Framework::StringFormatter::format_loc_str(NAME(lsOnSuccessBonus), Framework::StringFormatterParams()
			.add(NAME(bonusCoef), onSuccessBonus.as_string()));
	}
	if (!onComeBackBonus.is_empty())
	{
		if (!result.is_empty()) result += TXT("~");
		result += Framework::StringFormatter::format_loc_str(NAME(lsOnComeBackBonus), Framework::StringFormatterParams()
			.add(NAME(bonusCoef), onComeBackBonus.as_string()));
	}

	if (! result.is_empty())
	{
		result = LOC_STR(NAME(lsBonusInfo)) + TXT("~") + result;
	}

	return result;
}

//--

bool MissionDifficultyModifier::BonusCoef::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	
	experienceCoef = _node->get_float_attribute_or_from_child(TXT("experienceCoef"), experienceCoef);
	meritPointsCoef = _node->get_float_attribute_or_from_child(TXT("meritPointsCoef"), meritPointsCoef);

	return result;
}

bool MissionDifficultyModifier::BonusCoef::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;
	
	if (experienceCoef != 0.0f)
	{
		_node->set_float_attribute(TXT("experienceCoef"), experienceCoef);
	}

	if (meritPointsCoef != 0.0f)
	{
		_node->set_float_attribute(TXT("meritPointsCoef"), meritPointsCoef);
	}

	return result;
}

String MissionDifficultyModifier::BonusCoef::as_string() const
{
	String result;

	if (experienceCoef != 0.0f)
	{
		if (!result.is_empty()) result += String::space();
		result += experienceCoef > 0.0f? TXT("+") : TXT("-");
		result += ParserUtils::float_to_string_auto_decimals(abs(experienceCoef * 100.0f), 2);
		result += TXT("%");
		result += LOC_STR(NAME(lsCharsExperience));
	}

	if (meritPointsCoef != 0.0f)
	{
		if (!result.is_empty()) result += String::space();
		result += meritPointsCoef > 0.0f? TXT("+") : TXT("-");
		result += ParserUtils::float_to_string_auto_decimals(abs(meritPointsCoef * 100.0f), 2);
		result += TXT("%");
		result += LOC_STR(NAME(lsCharsMeritPoints));
	}


	return result;
}

String MissionDifficultyModifier::BonusCoef::as_applied_bonus_string() const
{
	String result;
	
	if (!is_empty())
	{
		result += Framework::StringFormatter::format_loc_str(NAME(lsAppliedBonus), Framework::StringFormatterParams()
			.add(NAME(bonusCoef), as_string()));
	}

	return result;
}
