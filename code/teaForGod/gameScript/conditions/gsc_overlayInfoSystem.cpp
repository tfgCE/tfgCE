#include "gsc_overlayInfoSystem.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::OverlayInfoSystem::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	isEmpty.load_from_xml(_node, TXT("isEmpty"));

	return result;
}

bool Conditions::OverlayInfoSystem::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (auto* ois = OverlayInfo::System::get())
	{
		if (isEmpty.is_set())
		{
			bool emptyNow = ois->is_empty();
			anyOk |= isEmpty.get() == emptyNow;
			anyFailed |= isEmpty.get() != emptyNow;
		}
	}

	return anyOk && ! anyFailed;
}
