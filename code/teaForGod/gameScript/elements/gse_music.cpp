#include "gse_music.h"

#include "..\..\game\playerSetup.h"
#include "..\..\music\musicPlayer.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Music::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	allowNonGameMusic.load_from_xml(_node, TXT("allowNonGameMusic"));
	
	requestNone = _node->get_bool_attribute_or_from_child_presence(TXT("requestNone"), requestNone);
	requestAmbient = _node->get_bool_attribute_or_from_child_presence(TXT("requestAmbient"), requestAmbient);
	requestCombatAuto = _node->get_bool_attribute_or_from_child_presence(TXT("requestCombatAuto"), requestCombatAuto);

	requestIncidental = _node->get_bool_attribute_or_from_child_presence(TXT("requestIncidental"), requestIncidental);
	requestIncidentalOverlay = _node->get_bool_attribute_or_from_child_presence(TXT("requestIncidentalOverlay"), requestIncidentalOverlay);
	requestIncidentalMusicTrack.load_from_xml(_node, TXT("requestIncidentalMusicTrack"));
	if (requestIncidentalMusicTrack.is_set() &&
		requestIncidentalMusicTrack.get().is_valid())
	{
		requestIncidental = true;
	}
	if (_node->has_attribute(TXT("requestIncidentalOverlayMusicTrack")))
	{
		requestIncidental = true;
		requestIncidentalOverlay = true;
		requestIncidentalMusicTrack.load_from_xml(_node, TXT("requestIncidentalOverlayMusicTrack"));
	}
	for_every(node, _node->children_named(TXT("requestIncidental")))
	{
		requestIncidental = true;
		requestIncidentalMusicTrack.load_from_xml(node, TXT("musicTrack"));
	}
	for_every(node, _node->children_named(TXT("requestIncidentalOverlay")))
	{
		requestIncidentalOverlay = true;
		requestIncidentalMusicTrack.load_from_xml(node, TXT("musicTrack"));
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("stopIncidental")))
	{
		// request without providing
		requestIncidentalOverlay = true;
		requestIncidental = true;
	}

	requestMusicTrack.load_from_xml(_node, TXT("requestMusicTrack"));

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Music::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (allowNonGameMusic.is_set())
	{
		if (auto* ps = PlayerSetup::access_current_if_exists())
		{
			ps->allow_non_game_music(allowNonGameMusic.get());
		}
	}
	if (requestNone)
	{
		MusicPlayer::request_none();
	}
	if (requestAmbient)
	{
		MusicPlayer::request_ambient();
	}
	if (requestIncidental || requestIncidentalOverlay)
	{
		MusicPlayer::request_incidental(requestIncidentalMusicTrack, requestIncidentalOverlay);
	}
	if (requestCombatAuto)
	{
		MusicPlayer::request_combat_auto();
	}
	if (requestMusicTrack.is_set())
	{
		if (requestMusicTrack.get().is_valid())
		{
			MusicPlayer::request_music_track(requestMusicTrack);
		}
		else
		{
			MusicPlayer::request_music_track(NP); // clear
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
