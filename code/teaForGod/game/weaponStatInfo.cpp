#include "weaponStatInfo.h"

#include "..\..\framework\text\localisedString.h"

//

using namespace TeaForGodEmperor;

//

// weapon stat affection symbols
DEFINE_STATIC_NAME_STR(lsCharsIncBetter, TXT("chars; inc better"));
DEFINE_STATIC_NAME_STR(lsCharsDecBetter, TXT("chars; dec better"));
DEFINE_STATIC_NAME_STR(lsCharsIncMuchBetter, TXT("chars; inc much better"));
DEFINE_STATIC_NAME_STR(lsCharsDecMuchBetter, TXT("chars; dec much better"));
DEFINE_STATIC_NAME_STR(lsCharsIncWorse, TXT("chars; inc worse"));
DEFINE_STATIC_NAME_STR(lsCharsDecWorse, TXT("chars; dec worse"));
DEFINE_STATIC_NAME_STR(lsCharsIncMuchWorse, TXT("chars; inc much worse"));
DEFINE_STATIC_NAME_STR(lsCharsDecMuchWorse, TXT("chars; dec much worse"));
DEFINE_STATIC_NAME_STR(lsCharsWeaponStatAffectionSet, TXT("chars; weapon stat affection set"));
DEFINE_STATIC_NAME_STR(lsCharsWeaponStatAffectionSpecial, TXT("chars; weapon stat affection special"));

//

struct WeaponStatInfoStrings
{
	bool ready = false;
	Framework::LocalisedString lsWeaponStatAffectionIncBetter;
	Framework::LocalisedString lsWeaponStatAffectionDecBetter;
	Framework::LocalisedString lsWeaponStatAffectionIncMuchBetter;
	Framework::LocalisedString lsWeaponStatAffectionDecMuchBetter;
	Framework::LocalisedString lsWeaponStatAffectionIncWorse;
	Framework::LocalisedString lsWeaponStatAffectionDecWorse;
	Framework::LocalisedString lsWeaponStatAffectionDecMuchWorse;
	Framework::LocalisedString lsWeaponStatAffectionIncMuchWorse;
	Framework::LocalisedString lsWeaponStatAffectionSet;
	Framework::LocalisedString lsWeaponStatAffectionSpecial;
};

WeaponStatInfoStrings* g_strings = nullptr;

void WeaponStatAffection::initialise_static()
{
	an_assert(! g_strings);
	g_strings = new WeaponStatInfoStrings();
}

void WeaponStatAffection::ready_to_use()
{
	if (g_strings->ready)
	{
		return;
	}
	// should remain as they are, no matter the language
	g_strings->lsWeaponStatAffectionIncBetter = ::Framework::LocalisedString(NAME(lsCharsIncBetter), true);
	g_strings->lsWeaponStatAffectionDecBetter = ::Framework::LocalisedString(NAME(lsCharsDecBetter), true);
	g_strings->lsWeaponStatAffectionIncMuchBetter = ::Framework::LocalisedString(NAME(lsCharsIncMuchBetter), true);
	g_strings->lsWeaponStatAffectionDecMuchBetter = ::Framework::LocalisedString(NAME(lsCharsDecMuchBetter), true);
	g_strings->lsWeaponStatAffectionIncWorse = ::Framework::LocalisedString(NAME(lsCharsIncWorse), true);
	g_strings->lsWeaponStatAffectionDecWorse = ::Framework::LocalisedString(NAME(lsCharsDecWorse), true);
	g_strings->lsWeaponStatAffectionDecMuchWorse = ::Framework::LocalisedString(NAME(lsCharsIncMuchWorse), true);
	g_strings->lsWeaponStatAffectionIncMuchWorse = ::Framework::LocalisedString(NAME(lsCharsDecMuchWorse), true);
	g_strings->lsWeaponStatAffectionSet = ::Framework::LocalisedString(NAME(lsCharsWeaponStatAffectionSet), true);
	g_strings->lsWeaponStatAffectionSpecial = ::Framework::LocalisedString(NAME(lsCharsWeaponStatAffectionSpecial), true);

	g_strings->ready = true;
}

void WeaponStatAffection::close_static()
{
	an_assert(g_strings);
	delete_and_clear(g_strings);
}

String const& WeaponStatAffection::to_ui_symbol(WeaponStatAffection::Type _type)
{
	switch (_type)
	{
	case WeaponStatAffection::IncBetter: return g_strings->lsWeaponStatAffectionIncBetter.get();
	case WeaponStatAffection::DecBetter: return g_strings->lsWeaponStatAffectionDecBetter.get();
	case WeaponStatAffection::IncMuchBetter: return g_strings->lsWeaponStatAffectionIncMuchBetter.get();
	case WeaponStatAffection::DecMuchBetter: return g_strings->lsWeaponStatAffectionDecMuchBetter.get();
	case WeaponStatAffection::IncWorse: return g_strings->lsWeaponStatAffectionIncWorse.get();
	case WeaponStatAffection::DecWorse: return g_strings->lsWeaponStatAffectionDecWorse.get();
	case WeaponStatAffection::IncMuchWorse: return g_strings->lsWeaponStatAffectionIncMuchWorse.get();
	case WeaponStatAffection::DecMuchWorse: return g_strings->lsWeaponStatAffectionDecMuchWorse.get();
	case WeaponStatAffection::Set: return g_strings->lsWeaponStatAffectionSet.get();
	case WeaponStatAffection::Special: return g_strings->lsWeaponStatAffectionSpecial.get();
	default: break;
	}

	return String::empty();
}
