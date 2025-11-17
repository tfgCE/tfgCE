#include "gse_waitForNoVoiceover.h"

#include "..\..\sound\voiceoverSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

Framework::GameScript::ScriptExecutionResult::Type WaitForNoVoiceover::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* vos = VoiceoverSystem::get())
	{
		if (vos->is_anyone_speaking())
		{
			return Framework::GameScript::ScriptExecutionResult::Repeat;
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
