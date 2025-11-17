#include "lhu_playerStats.h"

#include "..\loaderHubScreen.h"

#include "..\..\..\game\playerSetup.h"

//

DEFINE_STATIC_NAME_STR(lsRuns, TXT("hub; stats info; run count"));
DEFINE_STATIC_NAME_STR(lsRunTime, TXT("hub; stats info; run time"));
DEFINE_STATIC_NAME_STR(lsRunDistance, TXT("hub; stats info; run distance"));
DEFINE_STATIC_NAME_STR(lsDeaths, TXT("hub; stats info; death count"));
DEFINE_STATIC_NAME_STR(lsKills, TXT("hub; stats info; kill count"));
DEFINE_STATIC_NAME_STR(lsExperience, TXT("hub; stats info; experience"));
DEFINE_STATIC_NAME_STR(lsAllEXMs, TXT("hub; stats info; all exms"));

//

void Loader::Utils::add_options_from_player_stats(TeaForGodEmperor::PlayerStats const& _stats, Array<HubScreenOption>& _options, int _flags)
{
	auto& ps = TeaForGodEmperor::PlayerSetup::get_current();

	_options.push_back(HubScreenOption(Name::invalid(), NAME(lsRunTime), nullptr, HubScreenOption::Text).with_text(_stats.timeRun.get_simple_compact_hour_minute_second_string()));
	if (ps.get_preferences().get_measurement_system() == MeasurementSystem::Imperial)
	{
		_options.push_back(HubScreenOption(Name::invalid(), NAME(lsRunDistance), nullptr, HubScreenOption::Text).with_text(String::printf(TXT("%.1f mi"), MeasurementSystem::to_miles(_stats.distance.as_meters()))));
	}
	else
	{
		_options.push_back(HubScreenOption(Name::invalid(), NAME(lsRunDistance), nullptr, HubScreenOption::Text).with_text(String::printf(TXT("%.1f km"), MeasurementSystem::to_kilometers(_stats.distance.as_meters()))));
	}
	_options.push_back(HubScreenOption(Name::invalid(), NAME(lsExperience), nullptr, HubScreenOption::Text).with_text(_stats.experience.as_string_auto_decimals()).with_smaller_decimals(0.5f));
	_options.push_back(HubScreenOption(Name::invalid(), NAME(lsAllEXMs), nullptr, HubScreenOption::Text).with_text(String::printf(TXT("%i"), _stats.allEXMs)));
	_options.push_back(HubScreenOption(Name::invalid(), NAME(lsKills), nullptr, HubScreenOption::Text).with_text(String::printf(TXT("%i"), _stats.kills)));
	_options.push_back(HubScreenOption(Name::invalid(), NAME(lsDeaths), nullptr, HubScreenOption::Text).with_text(String::printf(TXT("%i"), _stats.deaths)));
}
