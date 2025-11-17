#include "aiPerceptionPause.h"

#include "..\..\core\debug\logInfoContext.h"

//
using namespace TeaForGodEmperor;
using namespace AI;

//

PerceptionPause PerceptionPause::s_empty;

PerceptionPause::PerceptionPause()
{
	SET_EXTRA_DEBUG_INFO(pauseReasons, TXT("PerceptionPause.pauseReasons"));
	SET_EXTRA_DEBUG_INFO(ignoreReasons, TXT("PerceptionPause.ignoreReasons"));
}

void PerceptionPause::log(LogInfoContext & _context) const
{
	_context.log(TXT("pause state: %S"), is_paused()? TXT("paused") : TXT("-"));
	_context.increase_indent();
	_context.increase_indent();
	for_every(pr, pauseReasons)
	{
		_context.log(TXT("%S"), pr->to_char());
	}
	_context.decrease_indent();
	if (!ignoreReasons.is_empty())
	{
		_context.log(TXT("ignore reasons"));
		_context.increase_indent();
		for_every(pr, ignoreReasons)
		{
			_context.log(TXT("%S"), pr->to_char());
		}
		_context.decrease_indent();
	}
	_context.decrease_indent();
}

