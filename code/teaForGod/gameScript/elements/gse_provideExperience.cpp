#include "gse_provideExperience.h"

#include "..\..\game\game.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool ProvideExperience::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	experience.load_from_xml(_node, TXT("experience"));

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type ProvideExperience::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (! experience.is_zero())
	{
		PlayerSetup::access_current().stats__experience(experience);
		GameStats::get().add_experience(experience);
		Persistence::access_current().provide_experience(experience);
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
