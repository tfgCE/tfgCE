#include "gsc_voiceoverSpeaking.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\sound\voiceoverSystem.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::VoiceoverSpeaking::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	actor.load_from_xml(_node, TXT("actor"));

	return result;
}

bool Conditions::VoiceoverSpeaking::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool result = false;

	if (auto* vs = VoiceoverSystem::get())
	{
		if (actor.is_set())
		{
			return vs->is_speaking(actor.get());
		}
		else
		{
			return vs->is_anyone_speaking();
		}
	}

	return result;
}
