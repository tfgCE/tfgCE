#include "gameStats.h"

#include "game.h"

#include "..\tutorials\tutorialSystem.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\io\dir.h"
#include "..\..\core\io\file.h"
#include "..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define AN_LOG_GAME_STATS
#endif

//

using namespace TeaForGodEmperor;

//

GameStats GameStats::s_statsTutorial;
GameStats GameStats::s_statsPilgrimage;
GameStats* GameStats::s_stats = &s_statsPilgrimage;

GameStats::GameStats()
{
	clear();
}

GameStats& GameStats::operator=(GameStats const& _src)
{
	Concurrency::ScopedSpinLock lock(statsLock);
	Concurrency::ScopedSpinLock srcLock(_src.statsLock);

	when = _src.when;
	size = _src.size;
	seed = _src.seed;

	distance = _src.distance;
	timeInSeconds = _src.timeInSeconds;
	deaths = _src.deaths;
	finished = _src.finished;
	kills = _src.kills;
	shotsFired = _src.shotsFired;
	itemsPickedUp = _src.itemsPickedUp;
	experience = _src.experience;
	experienceFromMission = _src.experienceFromMission;
	meritPoints = _src.meritPoints;

	return *this;
}

void GameStats::reset()
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	if (Game::get())
	{
		output(TXT("reset game stats"));
	}
#endif

	clear();
}

void GameStats::clear()
{
	Concurrency::ScopedSpinLock lock(statsLock, true);

	when = Framework::DateTime::get_current_from_system();
	size = String();
	seed = String();
	distance = 0.0f;
	timeInSeconds = 0;
	deaths = 0;
	finished = 0;
	kills = 0;
	shotsFired = 0;
	itemsPickedUp = 0;
	itemsPickedUpArray.clear();
	experience = Energy::zero();
	experienceFromMission = Energy::zero();
	meritPoints = 0;
}

void GameStats::add_experience(Energy const& _experience)
{
	Concurrency::ScopedSpinLock lock(statsLock);

	experience += _experience;

#ifdef AN_LOG_GAME_STATS
	output(TXT("[game stats] experience +%S [%S]"), _experience.as_string_auto_decimals().to_char(), experience.as_string_auto_decimals().to_char());
#endif
}

void GameStats::add_experience_from_mission(Energy const& _experienceFromMission)
{
	Concurrency::ScopedSpinLock lock(statsLock);

	experienceFromMission += _experienceFromMission;

#ifdef AN_LOG_GAME_STATS
	output(TXT("[game stats] experienceFromMission +%S [%S]"), _experienceFromMission.as_string_auto_decimals().to_char(), experienceFromMission.as_string_auto_decimals().to_char());
#endif
}

void GameStats::add_merit_points(int _meritPoints)
{
	Concurrency::ScopedSpinLock lock(statsLock);

	meritPoints += _meritPoints;

#ifdef AN_LOG_GAME_STATS
	output(TXT("[game stats] meritPoints +%i [%i]"), _meritPoints, meritPoints);
#endif
}

void GameStats::killed()
{
	Concurrency::ScopedSpinLock lock(statsLock);

	++kills;

#ifdef AN_LOG_GAME_STATS
	output(TXT("[game stats] killed +1 [%i]"), kills);
#endif
}

void GameStats::shot_fired()
{
	Concurrency::ScopedSpinLock lock(statsLock);

	++shotsFired;
}

void GameStats::picked_up_item(void* _item)
{
	Concurrency::ScopedSpinLock lock(statsLock);

	if (! itemsPickedUpArray.does_contain(_item))
	{
		itemsPickedUpArray.push_back_unique(_item);
		++itemsPickedUp;
	}
}

#define LOAD_FLOAT(what) what = node->get_float_attribute_or_from_child(TXT(#what), what);
#define LOAD_INT(what) what = node->get_int_attribute_or_from_child(TXT(#what), what);

#define SAVE_FLOAT(what) _node->set_float_to_child(TXT(#what), what);
#define SAVE_INT(what) _node->set_int_attribute(TXT(#what), what);

bool GameStats::load_from_xml_child_node(IO::XML::Node const* _node, tchar const * _childName)
{
	bool result = true;

	reset();

	for_every(node, _node->children_named(_childName))
	{
		when.load_from_xml(node->first_child_named(TXT("when")));
		LOAD_FLOAT(distance);
		LOAD_INT(timeInSeconds);
		LOAD_INT(deaths);
		LOAD_INT(finished);
		LOAD_INT(kills);
		LOAD_INT(shotsFired);
		LOAD_INT(itemsPickedUp);
		experience.load_from_xml(node, TXT("experience"));
		experienceFromMission.load_from_xml(node, TXT("experienceFromMission"));
		LOAD_INT(meritPoints);
	}

	return result;
}

bool GameStats::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	when.save_to_xml(_node->add_node(TXT("when")));

	SAVE_FLOAT(distance);
	SAVE_INT(timeInSeconds);
	SAVE_INT(deaths);
	SAVE_INT(finished);
	SAVE_INT(kills);
	SAVE_INT(shotsFired);
	SAVE_INT(itemsPickedUp);
	_node->set_attribute(TXT("experience"), experience.as_string_auto_decimals());
	_node->set_attribute(TXT("experienceFromMission"), experienceFromMission.as_string_auto_decimals());
	SAVE_INT(meritPoints);

	return result;
}
