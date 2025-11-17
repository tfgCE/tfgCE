#include "gsc_musicPlayer.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\music\musicPlayer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

// "holding" values
DEFINE_STATIC_NAME(upgradeCanister);

//

bool Conditions::MusicPlayer::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	isPlayingCombat.load_from_xml(_node, TXT("isPlayingCombat"));
	isPlayingTrack.load_from_xml(_node, TXT("isPlayingTrack"));

	return result;
}

bool Conditions::MusicPlayer::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (auto* mp = TeaForGodEmperor::MusicPlayer::get())
	{
		if (isPlayingCombat.is_set())
		{
			bool matches = isPlayingCombat.get() == mp->is_playing_combat();
			anyOk |= matches;
			anyFailed |= !matches;
		}
		if (isPlayingTrack.is_set())
		{
			bool matches = isPlayingTrack.get() == mp->get_current_track();
			anyOk |= matches;
			anyFailed |= !matches;
		}
	}

	return anyOk && !anyFailed;
}
