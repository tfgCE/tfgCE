#include "gse_door.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\world\door.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Door::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	openInTime.load_from_xml(_node, TXT("openInTime"));
	extendOpenInTimeByPeriodNum.load_from_xml(_node, TXT("extendOpenInTimeByPeriodNum"));
	closeInTime.load_from_xml(_node, TXT("closeInTime"));
	overrideLock.load_from_xml(_node, TXT("overrideLock"));

	openInTimePeriod.load_from_xml(_node, TXT("openInTimePeriod"));
	openInTimePeriod.load_from_xml(_node, TXT("period"));
	openingSound.load_from_xml(_node, TXT("openingSound"), _lc);
	openSound.load_from_xml(_node, TXT("openSound"), _lc);
	closedSound.load_from_xml(_node, TXT("closedSound"), _lc);

	maxTimeToExtend.load_from_xml(_node, TXT("maxTimeToExtend"));

	return result;
}

bool Door::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= openingSound.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= openSound.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= closedSound.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Door::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	auto result = base::execute(_execution, _flags);

	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Door>>(doorVar))
	{
		if (auto* d = exPtr->get())
		{
			if (openInTime.is_set())
			{
				TeaForGodEmperor::GameDirector::DoorOpeningParams dop;
				dop.with_period(openInTimePeriod);
				dop.with_opening_sound(openingSound.get());
				dop.with_open_sound(openSound.get());
				TeaForGodEmperor::GameDirector::get()->set_door_opening(d, openInTime.get(), dop);
			}
			if (extendOpenInTimeByPeriodNum.is_set())
			{
				TeaForGodEmperor::GameDirector::get()->extend_door_opening(d, extendOpenInTimeByPeriodNum.get(), maxTimeToExtend);
			}
			if (closeInTime.is_set())
			{
				TeaForGodEmperor::GameDirector::DoorClosingParams dcp;
				dcp.with_closed_sound(closedSound.get());
				TeaForGodEmperor::GameDirector::get()->set_door_closing(d, closeInTime.get(), dcp);
			}
			if (overrideLock.is_set())
			{
				TeaForGodEmperor::GameDirector::get()->set_door_override_lock(d, overrideLock.get());
			}
		}
	}
	else
	{
		warn(TXT("missing object"));
	}

	return result;
}
